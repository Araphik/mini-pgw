#include "../include/server/pgw_server.hpp"

PgwServer::PgwServer(const ServerConfig& config)
    : config_(config),
      cdr_writer_(config.cdr_file),
      running_(true),
      pool_running_(true),
      blacklist_(config.blacklist.begin(), config.blacklist.end()) {}

PgwServer::~PgwServer() {
    pool_running_ = false;
    queue_cond_.notify_all();
    for (auto& thread : thread_pool_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

int PgwServer::setup_socket() {
    if (config_.udp_port <= 0 || config_.udp_port > 65535) {
        spdlog::error("Invalid UDP port: {}", config_.udp_port);
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (sockfd < 0) {
        spdlog::error("socket creation failed: {}", strerror(errno));
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config_.udp_port);
    if (inet_pton(AF_INET, config_.udp_ip.c_str(), &server_addr.sin_addr) <= 0) {
        spdlog::error("Invalid IP address: {}", config_.udp_ip);
        close(sockfd);
        return -1;
    }

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        spdlog::error("bind failed: {}", strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int PgwServer::setup_epoll(int sockfd) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        spdlog::critical("epoll_create1 failed: {}", strerror(errno));
        return -1;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        spdlog::critical("epoll_ctl failed: {}", strerror(errno));
        close(epoll_fd);
        return -1;
    }
    return epoll_fd;
}

void PgwServer::start_session_cleaner() {
    cleaner_thread_ = std::thread([&]() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto now = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lock(session_mutex_);
            for (auto it = session_table_.begin(); it != session_table_.end(); ) {
                if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count() > config_.session_timeout_sec) {
                    cdr_writer_.write(it->first, "timeout");
                    spdlog::info("Session expired for IMSI {}", it->first);
                    it = session_table_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    });
}

void PgwServer::start_thread_pool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
        thread_pool_.emplace_back(&PgwServer::worker_thread, this);
    }
    spdlog::info("Started thread pool with {} threads", num_threads);
}

void PgwServer::worker_thread() {
    while (pool_running_) {
        ClientTask task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cond_.wait(lock, [this] { return !task_queue_.empty() || !pool_running_; });
            if (!pool_running_ && task_queue_.empty()) {
                return;
            }
            task = std::move(task_queue_.front());
            task_queue_.pop();
        }
        handle_client(task.sockfd, task.client_addr, task.buffer);
    }
}

void PgwServer::handle_client(int sockfd, const sockaddr_in& client_addr, const std::vector<uint8_t>& buffer) {
    std::string imsi = decoder_.decode(buffer);
    spdlog::info("Received IMSI: {}", imsi);

    if (imsi.length() != 15 || imsi.find_first_not_of("0123456789") != std::string::npos) {
        spdlog::warn("Received invalid IMSI '{}', ignoring", imsi);
        return;
    }

    std::string response;
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        if (blacklist_.count(imsi)) {
            response = "rejected";
            spdlog::warn("IMSI {} is blacklisted", imsi);
        } else if (session_table_.count(imsi)) {
            response = "created";
            spdlog::info("IMSI {} already has session", imsi);
        } else {
            response = "created";
            session_table_[imsi] = std::chrono::steady_clock::now();
            cdr_writer_.write(imsi, "create");
            spdlog::info("Session created for IMSI {}", imsi);
        }
    }
    sendto(sockfd, response.c_str(), response.size(), 0,
           (sockaddr*)&client_addr, sizeof(client_addr));
    spdlog::debug("Sent response: {}", response);
}

void PgwServer::run() {
    int sockfd = setup_socket();
    if (sockfd == -1) return;

    int epoll_fd = setup_epoll(sockfd);
    if (epoll_fd == -1) {
        close(sockfd);
        return;
    }

    int event_fd = eventfd(0, EFD_NONBLOCK);
    if (event_fd == -1) {
        spdlog::critical("eventfd creation failed: {}", strerror(errno));
        close(sockfd);
        close(epoll_fd);
        return;
    }

    epoll_event ev_event{};
    ev_event.events = EPOLLIN;
    ev_event.data.fd = event_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &ev_event);

    HttpServer http(config_, running_, session_table_, session_mutex_, event_fd);
    http.run();

    start_session_cleaner();
    start_thread_pool(std::thread::hardware_concurrency());

    spdlog::info("UDP server started with epoll on {}:{}", config_.udp_ip, config_.udp_port);

    const int MAX_EVENTS = 1000;
    epoll_event events[MAX_EVENTS];
    bool should_exit = false;

    while (!should_exit) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            spdlog::error("epoll_wait failed: {}", strerror(errno));
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == event_fd) {
                uint64_t dummy;
                ssize_t bytes_read = read(event_fd, &dummy, sizeof(dummy));
                if (bytes_read != sizeof(dummy)) {
                    spdlog::error("Failed to read from event_fd (expected {}, got {})", sizeof(dummy), bytes_read);
                }
                spdlog::info("Shutdown event received from HTTP thread");
                should_exit = true;
                break;
            }

            if (events[i].data.fd == sockfd) {
                std::vector<uint8_t> buffer(16);
                sockaddr_in client_addr{};
                socklen_t len = sizeof(client_addr);

                ssize_t n = recvfrom(sockfd, buffer.data(), buffer.size(), 0,
                                     (sockaddr*)&client_addr, &len);
                if (n <= 0) continue;

                buffer.resize(n);
                {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    task_queue_.push({sockfd, client_addr, buffer});
                }
                queue_cond_.notify_one();
            }
        }
    }

    pool_running_ = false;
    queue_cond_.notify_all();
    for (auto& thread : thread_pool_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    cleaner_thread_.join();
    http.stop();
    close(epoll_fd);
    close(sockfd);
    close(event_fd);
    spdlog::info("UDP server stopped");
}