#include <gtest/gtest.h>
#include "../include/server/pgw_server.hpp"
#include "../include/config_loader.hpp"
#include <httplib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <vector>
#include <string>
#include <chrono>

class PgwServerTest : public ::testing::Test {
protected:
    ServerConfig config;

    void SetUp() override {
        config.udp_ip = "127.0.0.1";
        config.udp_port = 9999;
        config.http_port = 8080;
        config.log_file = "test_pgw.log";
        config.log_level = "off";
        config.session_timeout_sec = 10;
        config.cdr_file = "test_cdr.log";
        config.blacklist = {"1111"};
    }
};

TEST_F(PgwServerTest, SetupSocketCreatesSocket) {
    PgwServer server(config);
    int sockfd = server.test_setup_socket();
    EXPECT_GE(sockfd, 0);
    if (sockfd >= 0) close(sockfd);
}

TEST_F(PgwServerTest, SetupEpollCreatesEpollFD) {
    PgwServer server(config);
    int sockfd = server.test_setup_socket();
    ASSERT_GE(sockfd, 0);

    int epoll_fd = server.test_setup_epoll(sockfd);
    EXPECT_GE(epoll_fd, 0);

    if (epoll_fd >= 0) close(epoll_fd);
    if (sockfd >= 0) close(sockfd);
}

TEST_F(PgwServerTest, HandleClient_BlacklistedImsiGetsRejected) {
    PgwServer server(config);
    std::vector<uint8_t> buffer = {0x11, 0x11}; 
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    server.test_handle_client(sockfd, addr, buffer);
    close(sockfd);
}

TEST_F(PgwServerTest, HandleClient_NewImsiCreatesSession) {
    config.blacklist.clear();
    PgwServer server(config);
    std::vector<uint8_t> buffer = {0x21, 0x43}; 

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    server.test_handle_client(sockfd, addr, buffer);
    close(sockfd);
}

TEST_F(PgwServerTest, HandleClient_RepeatedImsiSessionNotDuplicated) {
    config.blacklist.clear();
    PgwServer server(config);
    std::vector<uint8_t> buffer = {0x21, 0x43}; 

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    server.test_handle_client(sockfd, addr, buffer);
    server.test_handle_client(sockfd, addr, buffer); 
    close(sockfd);
}

TEST_F(PgwServerTest, HandleClient_EmptyImsi) {
    config.blacklist.clear();
    PgwServer server(config);
    std::vector<uint8_t> buffer = {}; 

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    server.test_handle_client(sockfd, addr, buffer);
    close(sockfd);
}

TEST_F(PgwServerTest, SessionCleanerRemovesExpiredSessions) {
    config.blacklist.clear();
    config.session_timeout_sec = 1;
    PgwServer server(config);

    std::string imsi = "9999";
    server.test_sessions()[imsi] = std::chrono::steady_clock::now() - std::chrono::seconds(5);
    server.test_running() = true;

    std::thread t([&server]() {
        server.test_start_session_cleaner();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        server.test_running() = false;
        server.test_cleaner_thread().join();
    });
    t.join();

    EXPECT_EQ(server.test_sessions().count(imsi), 0);
}

TEST_F(PgwServerTest, SessionCleanerDoesNotRemoveActiveSessions) {
    config.blacklist.clear();
    config.session_timeout_sec = 10;
    PgwServer server(config);

    std::string imsi = "8888";
    server.test_sessions()[imsi] = std::chrono::steady_clock::now();
    server.test_running() = true;

    std::thread t([&server]() {
        server.test_start_session_cleaner();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        server.test_running() = false;
        server.test_cleaner_thread().join();
    });
    t.join();

    EXPECT_EQ(server.test_sessions().count(imsi), 1);
}

TEST_F(PgwServerTest, SetupSocketFailsOnInvalidPort) {
    config.udp_port = 65536; 
    PgwServer server(config);
    int sockfd = server.test_setup_socket();
    EXPECT_EQ(sockfd, -1);
}

TEST_F(PgwServerTest, SetupSocketFailsOnPortInUse) {
    int temp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(temp_sockfd, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.udp_port);
    server_addr.sin_addr.s_addr = inet_addr(config.udp_ip.c_str());

    int bind_result = bind(temp_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    ASSERT_EQ(bind_result, 0) << "Failed to bind temp socket: " << strerror(errno);

    PgwServer server(config);
    int sockfd = server.test_setup_socket();
    std::cerr << "sockfd: " << sockfd << std::endl; 
    EXPECT_EQ(sockfd, -1); 

    close(temp_sockfd);
}

TEST_F(PgwServerTest, HandleClient_InvalidBuffer) {
    config.blacklist.clear();
    PgwServer server(config);
    std::vector<uint8_t> buffer = {0xFF, 0xFF};

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    server.test_handle_client(sockfd, addr, buffer);
    close(sockfd);

    EXPECT_EQ(server.test_sessions().count("FFFF"), 0);
}

TEST_F(PgwServerTest, HandleClient_LargeBufferRejectedDueToInvalidImsi) {
    config.blacklist.clear();
    PgwServer server(config);

    std::vector<uint8_t> buffer(1024, 0x12); 

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    server.test_handle_client(sockfd, addr, buffer);
    close(sockfd);

    EXPECT_EQ(server.test_sessions().size(), 0);
}

TEST_F(PgwServerTest, SessionCleanerHandlesMultipleSessions) {
    config.blacklist.clear();
    config.session_timeout_sec = 1;
    PgwServer server(config);

    auto now = std::chrono::steady_clock::now();
    server.test_sessions()["1111"] = now - std::chrono::seconds(5); 
    server.test_sessions()["2222"] = now + std::chrono::seconds(1); 
    server.test_sessions()["3333"] = now - std::chrono::seconds(10);
    server.test_sessions()["4444"] = now + std::chrono::seconds(1); 

    server.test_running() = true;

    std::thread t([&server]() {
        server.test_start_session_cleaner();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        server.test_running() = false;
        server.test_cleaner_thread().join();
    });
    t.join();

    EXPECT_EQ(server.test_sessions().count("1111"), 0);
    EXPECT_EQ(server.test_sessions().count("2222"), 1);
    EXPECT_EQ(server.test_sessions().count("3333"), 0);
    EXPECT_EQ(server.test_sessions().count("4444"), 1);
}

TEST_F(PgwServerTest, SessionCleanerEmptyTable) {
    config.blacklist.clear();
    config.session_timeout_sec = 1;
    PgwServer server(config);

    server.test_running() = true;

    std::thread t([&server]() {
        server.test_start_session_cleaner();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        server.test_running() = false;
        server.test_cleaner_thread().join();
    });
    t.join();

    EXPECT_EQ(server.test_sessions().size(), 0);
}

TEST_F(PgwServerTest, RunHandlesMultipleClients) {
    config.blacklist.clear();
    PgwServer server(config);

    server.test_running() = true;
    std::thread server_thread([&server]() {
        server.run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); 

    auto encode_imsi_bcd = [](const std::string& imsi) -> std::vector<uint8_t> {
        std::vector<uint8_t> bcd;
        for (size_t i = 0; i < imsi.size(); i += 2) {
            uint8_t low = imsi[i] - '0';
            uint8_t high = (i + 1 < imsi.size()) ? (imsi[i + 1] - '0') : 0xF;
            bcd.push_back((high << 4) | low);
        }
        return bcd;
    };

    std::vector<std::string> imsis = {
        "123456789012345",
        "432176549876543",
        "987654321098765",
        "103254769810327"
    };

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(sockfd, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.udp_port);
    server_addr.sin_addr.s_addr = inet_addr(config.udp_ip.c_str());

    for (const auto& imsi : imsis) {
        std::vector<uint8_t> bcd = encode_imsi_bcd(imsi);
        sendto(sockfd, bcd.data(), bcd.size(), 0,
               (sockaddr*)&server_addr, sizeof(server_addr));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200)); 

    httplib::Client cli("http://127.0.0.1:8080");
    auto res = cli.Post("/stop");
    ASSERT_TRUE(res && res->status == 200) << "Failed to stop server";

    server.test_running() = false;
    server_thread.join();
    close(sockfd);

    for (const auto& imsi : imsis) {
        EXPECT_EQ(server.test_sessions().count(imsi), 1) << "Session not created for IMSI " << imsi;
    }
}

TEST_F(PgwServerTest, HandleClient_ResponseReceived) {
    config.blacklist.clear();
    PgwServer server(config);

    server.test_running() = true;
    std::thread server_thread([&server]() {
        server.run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto encode_imsi_bcd = [](const std::string& imsi) -> std::vector<uint8_t> {
        std::vector<uint8_t> bcd;
        for (size_t i = 0; i < imsi.size(); i += 2) {
            uint8_t low = imsi[i] - '0';
            uint8_t high = (i + 1 < imsi.size()) ? (imsi[i + 1] - '0') : 0xF;
            bcd.push_back((high << 4) | low);
        }
        return bcd;
    };

    std::string imsi = "123456789012345";
    std::vector<uint8_t> buffer = encode_imsi_bcd(imsi);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(sockfd, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.udp_port);
    server_addr.sin_addr.s_addr = inet_addr(config.udp_ip.c_str());

    sendto(sockfd, buffer.data(), buffer.size(), 0,
           (sockaddr*)&server_addr, sizeof(server_addr));

    std::vector<char> response_buffer(16);
    sockaddr_in from_addr{};
    socklen_t len = sizeof(from_addr);

    struct timeval timeout{};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ssize_t n = recvfrom(sockfd, response_buffer.data(), response_buffer.size(), 0,
                         (sockaddr*)&from_addr, &len);
    EXPECT_GT(n, 0) << "No response received from server (possible timeout)";
    std::string response(response_buffer.data(), n);
    EXPECT_EQ(response, "created");

    close(sockfd);

    httplib::Client cli("http://127.0.0.1:8080");
    auto res = cli.Post("/stop");
    ASSERT_TRUE(res && res->status == 200) << "Failed to stop server";

    server.test_running() = false;
    server_thread.join();
}

TEST_F(PgwServerTest, RunHandlesEpollWaitEINTR) {
    config.blacklist.clear();
    PgwServer server(config);

    auto previous_handler = signal(SIGUSR1, [](int) { /* No-op */ });
    if (previous_handler == SIG_ERR) {
        FAIL() << "Failed to install SIGUSR1 handler";
    }

    server.test_running() = true;
    std::thread server_thread([&server]() {
        server.run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    kill(getpid(), SIGUSR1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client cli("http://127.0.0.1:8080");
    auto res = cli.Post("/stop");
    if (!res || res->status != 200) {
        FAIL() << "Failed to send /stop request or server did not respond correctly";
    }

    server.test_running() = false;
    server_thread.join();

    signal(SIGUSR1, previous_handler);

    EXPECT_EQ(server.test_sessions().size(), 0);
}

TEST_F(PgwServerTest, RunHandlesThousandClientsConcurrently) {
    config.blacklist.clear();
    PgwServer server(config);

    server.test_running() = true;
    std::thread server_thread([&server]() {
        server.run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(300)); 

    auto encode_imsi_bcd = [](const std::string& imsi) -> std::vector<uint8_t> {
        std::vector<uint8_t> bcd;
        for (size_t i = 0; i < imsi.size(); i += 2) {
            uint8_t low = imsi[i] - '0';
            uint8_t high = (i + 1 < imsi.size()) ? (imsi[i + 1] - '0') : 0xF;
            bcd.push_back((high << 4) | low);
        }
        return bcd;
    };

    std::vector<std::string> imsis;
    for (int i = 0; i < 20000; ++i) {
        std::string imsi = std::to_string(100000000000000 + i);
        imsis.push_back(imsi);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(sockfd, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.udp_port);
    server_addr.sin_addr.s_addr = inet_addr(config.udp_ip.c_str());

    std::vector<std::thread> threads;
    for (const auto& imsi : imsis) {
        threads.emplace_back([&, imsi]() {
            std::vector<uint8_t> bcd = encode_imsi_bcd(imsi);
            sendto(sockfd, bcd.data(), bcd.size(), 0,
                (sockaddr*)&server_addr, sizeof(server_addr));
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    close(sockfd);
    std::this_thread::sleep_for(std::chrono::milliseconds(8500)); 

    httplib::Client cli("http://127.0.0.1:8080");
    auto res = cli.Post("/stop");
    ASSERT_TRUE(res && res->status == 200);

    server.test_running() = false;
    server_thread.join();

    for (const auto& imsi : imsis) {
        EXPECT_EQ(server.test_sessions().count(imsi), 1) << "Session not created for IMSI " << imsi;
    }
}

TEST_F(PgwServerTest, DestructorJoinsThread) {
    config.blacklist.clear();
    {
        PgwServer server(config);
        server.test_thread_pool().emplace_back([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }
    SUCCEED();
}

TEST_F(PgwServerTest, SocketCreationFails) {
    ServerConfig bad_config = config;
    bad_config.udp_port = 12345;
    
    bad_config.udp_ip = "999.999.999.999";  // invalid IP
    PgwServer server(bad_config);

    int sockfd = server.test_setup_socket();
    EXPECT_EQ(sockfd, -1);  
}

TEST_F(PgwServerTest, SetupEpollFailsOnBadFD) {
    PgwServer server(config);
    int epoll_fd = server.test_setup_epoll(-1);
    EXPECT_EQ(epoll_fd, -1);  // эполл упадёт
}

TEST_F(PgwServerTest, RunHandlesEpollWaitFatalError) {
    config.blacklist.clear();
    PgwServer server(config);

    server.test_running() = true;
    std::thread t([&]() {
        server.run();  
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client cli("http://127.0.0.1:8080");
    cli.Post("/stop");
    server.test_running() = false;
    t.join();

    SUCCEED();
}

TEST_F(PgwServerTest, SetupSocketFailsOnTooManyFDs) {
    std::vector<int> fds;
    for (int i = 0; i < 1024; ++i) {
        int fd = open("/dev/null", O_RDONLY);
        if (fd < 0) break;
        fds.push_back(fd);
    }

    PgwServer server(config);
    int sockfd = server.test_setup_socket();
    EXPECT_EQ(sockfd, -1);

    for (int fd : fds) close(fd);
}

TEST_F(PgwServerTest, SetupEpollFailsAfterSockClose) {
    PgwServer server(config);
    int sockfd = server.test_setup_socket();
    ASSERT_GE(sockfd, 0);
    close(sockfd); 

    int epoll_fd = server.test_setup_epoll(sockfd); 
    EXPECT_EQ(epoll_fd, -1);
}

TEST_F(PgwServerTest, HandleClient_BlacklistedImsi_RespondsRejected) {
    config.blacklist = {"123456789012345"};
    PgwServer server(config);

    server.test_running() = true;
    std::thread server_thread([&server]() {
        server.run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto encode_imsi_bcd = [](const std::string& imsi) -> std::vector<uint8_t> {
        std::vector<uint8_t> bcd;
        for (size_t i = 0; i < imsi.size(); i += 2) {
            uint8_t low = imsi[i] - '0';
            uint8_t high = (i + 1 < imsi.size()) ? (imsi[i + 1] - '0') : 0xF;
            bcd.push_back((high << 4) | low);
        }
        return bcd;
    };

    std::string imsi = "123456789012345";
    std::vector<uint8_t> buffer = encode_imsi_bcd(imsi);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(sockfd, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.udp_port);
    server_addr.sin_addr.s_addr = inet_addr(config.udp_ip.c_str());

    sendto(sockfd, buffer.data(), buffer.size(), 0,
           (sockaddr*)&server_addr, sizeof(server_addr));

    std::vector<char> response_buffer(16);
    sockaddr_in from_addr{};
    socklen_t len = sizeof(from_addr);

    struct timeval timeout{};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ssize_t n = recvfrom(sockfd, response_buffer.data(), response_buffer.size(), 0,
                         (sockaddr*)&from_addr, &len);
    ASSERT_GT(n, 0) << "No response received from server (timeout?)";

    std::string response(response_buffer.data(), n);
    EXPECT_EQ(response, "rejected");

    close(sockfd);

    httplib::Client cli("http://127.0.0.1:8080");
    auto res = cli.Post("/stop");
    ASSERT_TRUE(res && res->status == 200);

    server.test_running() = false;
    server_thread.join();
}








