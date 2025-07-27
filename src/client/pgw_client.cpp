#include "../include/config_loader.hpp"
#include "../include/client/bcd_encoder.hpp"
#include "../include/client/pgw_client.hpp"

ImsiClient::ImsiClient(const ClientConfig& config,
                       bool fail_socket,
                       bool fail_sendto,
                       bool fail_select,
                       bool fail_recv)
    : config_(config),
      fail_socket_(fail_socket),
      fail_sendto_(fail_sendto),
      fail_select_(fail_select),
      fail_recv_(fail_recv) {}

void ImsiClient::send_imsi_request(const std::string& imsi) {
    spdlog::info("Sending IMSI: {}", imsi);

    BcdEncoder encoder;
    auto bcd = encoder.encode(imsi);

    int sockfd = fail_socket_ ? -1 : socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        spdlog::error("Failed to create socket");
        return;
    }

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config_.server_port);
    if (inet_pton(AF_INET, config_.server_ip.c_str(), &server_addr.sin_addr) != 1) {
        spdlog::error("Invalid server IP address: {}", config_.server_ip);
        close(sockfd);
        return;
    }

    int attempt = 0;
    bool success = false;
    char buffer[128];

    while (attempt < MAX_RETRIES && !success) {
        attempt++;
        spdlog::info("Attempt {} of {}", attempt, MAX_RETRIES);

        ssize_t sent = fail_sendto_ ? -1 : sendto(sockfd, bcd.data(), bcd.size(), 0,
                                          (sockaddr*)&server_addr, sizeof(server_addr));

        if (sent < 0) {
            spdlog::error("Failed to send IMSI on attempt {}", attempt);
            close(sockfd);
            return;
        }

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int ready = fail_select_ ? -1 : select(sockfd + 1, &read_fds, nullptr, nullptr, &timeout);

        if (ready < 0) {
            spdlog::error("Select error on attempt {}", attempt);
            close(sockfd);
            return;
        } else if (ready == 0) {
            spdlog::warn("Timeout on attempt {}", attempt);
            if (attempt == MAX_RETRIES) {
                spdlog::error("Max retries reached, giving up");
                close(sockfd);
                return;
            }
            continue;
        }

        ssize_t recv_len = fail_recv_ ? -1 : recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);

        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            spdlog::info("Received response: {}", buffer);
            success = true;
        } else {
            spdlog::warn("No response received on attempt {}", attempt);
        }
        if (attempt == MAX_RETRIES || !success) {
            spdlog::error("Max retries reached, giving up");
            close(sockfd);
            return;
        }
    }

    close(sockfd);
}