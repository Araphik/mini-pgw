#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ostream_sink.h>
#include <fcntl.h>

#include "../../../include/client/pgw_client.hpp"
#include "../../../include/config_loader.hpp"

using namespace std::chrono_literals;

class DummyUdpServer {
public:
    DummyUdpServer(int port, std::string response = "ok")
        : port_(port), response_(std::move(response)), stop_(false) {}

    void start() {
        server_thread_ = std::thread([this]() {
            int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sockfd < 0) {
                perror("socket");
                return;
            }

            fcntl(sockfd, F_SETFL, O_NONBLOCK);

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port_);
            addr.sin_addr.s_addr = INADDR_ANY;
            if (bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
                perror("bind");
                return;
            }

            char buffer[128];
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);

            while (!stop_) {
                ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                            (sockaddr*)&client_addr, &client_len);
                if (recv_len > 0) {
                    sendto(sockfd, response_.c_str(), response_.size(), 0,
                           (sockaddr*)&client_addr, client_len);
                }
                std::this_thread::sleep_for(10ms);
            }
            close(sockfd);
        });

        std::this_thread::sleep_for(100ms);
    }

    void stop() {
        stop_ = true;
        if (server_thread_.joinable())
            server_thread_.join();
    }

private:
    int port_;
    std::string response_;
    bool stop_;
    std::thread server_thread_;
};

ClientConfig valid_config() {
    ClientConfig config;
    config.server_ip = "127.0.0.1";
    config.server_port = 9000;
    config.log_file = "client.log";
    config.log_level = "info";
    return config;
}

class PgwClientTest : public ::testing::Test {
protected:
    std::ostringstream log_stream;

    void SetUp() override {
        auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(log_stream);
        auto logger = std::make_shared<spdlog::logger>("test_logger", ostream_sink);
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::info);
    }

    std::string get_logs() {
        return log_stream.str();
    }
};

TEST_F(PgwClientTest, SendsAndReceivesSuccessfully) {
    DummyUdpServer server(9999);
    server.start();

    ClientConfig config = {"127.0.0.1", 9999, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("123456789012345");

    server.stop();
    EXPECT_NE(get_logs().find("Received response"), std::string::npos);
}

TEST_F(PgwClientTest, RetriesOnTimeoutAndFails) {
    ClientConfig config = {"127.0.0.1", 9998, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("123456789012345");

    EXPECT_NE(get_logs().find("Max retries reached, giving up"), std::string::npos);
}

TEST_F(PgwClientTest, InvalidIpFailsGracefully) {
    ClientConfig config = {"256.256.256.256", 9999, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("123456789012345");

    EXPECT_NE(get_logs().find("Invalid server IP address"), std::string::npos);
}

TEST_F(PgwClientTest, LogsImsiBeingSent) {
    DummyUdpServer server(9997);
    server.start();

    ClientConfig config = {"127.0.0.1", 9997, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("111111111111111");

    server.stop();
    EXPECT_NE(get_logs().find("Sending IMSI: 111111111111111"), std::string::npos);
}

TEST_F(PgwClientTest, HandlesEmptyImsi) {
    ClientConfig config = {"127.0.0.1", 9997, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("");

    EXPECT_NE(get_logs().find("Sending IMSI: "), std::string::npos);
}

TEST_F(PgwClientTest, EncodesImsiWithOddLength) {
    DummyUdpServer server(9996);
    server.start();

    ClientConfig config = {"127.0.0.1", 9996, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("12345");

    server.stop();
    EXPECT_NE(get_logs().find("Received response"), std::string::npos);
}

TEST_F(PgwClientTest, SendsToCorrectPort) {
    DummyUdpServer server(9995);
    server.start();

    ClientConfig config = {"127.0.0.1", 9995, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("987654321012345");

    server.stop();
    EXPECT_NE(get_logs().find("Received response"), std::string::npos);
}

TEST_F(PgwClientTest, HandlesAllZeroImsi) {
    DummyUdpServer server(9994);
    server.start();

    ClientConfig config = {"127.0.0.1", 9994, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("000000000000000");

    server.stop();
    EXPECT_NE(get_logs().find("Received response"), std::string::npos);
}

TEST_F(PgwClientTest, HandlesAllNineImsi) {
    DummyUdpServer server(9993);
    server.start();

    ClientConfig config = {"127.0.0.1", 9993, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("999999999999999");

    server.stop();
    EXPECT_NE(get_logs().find("Received response"), std::string::npos);
}


TEST_F(PgwClientTest, FailsOnSocketCreation) {
    ClientConfig config = {"127.0.0.1", 12345, "log.txt", "info"};
    ImsiClient client(config, true); 
    client.send_imsi_request("123456789012345");

    EXPECT_NE(get_logs().find("Failed to create socket"), std::string::npos);
}

TEST_F(PgwClientTest, FailsOnSendto) {
    ClientConfig config = {"127.0.0.1", 12345, "log.txt", "info"};
    ImsiClient client(config, false, true); 
    client.send_imsi_request("123456789012345");

    EXPECT_NE(get_logs().find("Failed to send IMSI"), std::string::npos);
}

TEST_F(PgwClientTest, FailsOnSelect) {
    ClientConfig config = {"127.0.0.1", 12345, "log.txt", "info"};
    ImsiClient client(config, false, false, true); 
    client.send_imsi_request("123456789012345");

    EXPECT_NE(get_logs().find("Select error on attempt"), std::string::npos);
}

TEST_F(PgwClientTest, FailsOnRecvfrom) {
    ClientConfig config = {"127.0.0.1", 12345, "log.txt", "info"};
    ImsiClient client(config, false, false, false, true); 
    client.send_imsi_request("123456789012345");

    auto logs = get_logs();
    EXPECT_NE(logs.find("Max retries reached"), std::string::npos);
}

TEST_F(PgwClientTest, HandlesMaxValidImsiLength) {
    DummyUdpServer server(9992);
    server.start();

    ClientConfig config = {"127.0.0.1", 9992, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("123456789012345");  

    server.stop();
    EXPECT_NE(get_logs().find("Received response"), std::string::npos);
}

TEST_F(PgwClientTest, HandlesShortImsiLength) {
    DummyUdpServer server(9991);
    server.start();

    ClientConfig config = {"127.0.0.1", 9991, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("12");

    server.stop();
    EXPECT_NE(get_logs().find("Received response"), std::string::npos);
}

TEST_F(PgwClientTest, HandlesSingleDigitImsi) {
    DummyUdpServer server(9990);
    server.start();

    ClientConfig config = {"127.0.0.1", 9990, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("7");

    server.stop();
    EXPECT_NE(get_logs().find("Received response"), std::string::npos);
}

TEST_F(PgwClientTest, SendImsiTwiceWithSameClient) {
    DummyUdpServer server(9989);
    server.start();

    ClientConfig config = {"127.0.0.1", 9989, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("123456789012345");
    client.send_imsi_request("543210987654321");

    server.stop();
    auto logs = get_logs();
    EXPECT_NE(logs.find("123456789012345"), std::string::npos);
    EXPECT_NE(logs.find("543210987654321"), std::string::npos);
}

TEST_F(PgwClientTest, SendImsiTwiceWithDifferentClients) {
    DummyUdpServer server(9988);
    server.start();

    ClientConfig config = {"127.0.0.1", 9988, "log.txt", "info"};
    ImsiClient client1(config);
    ImsiClient client2(config);

    client1.send_imsi_request("111111111111111");
    client2.send_imsi_request("222222222222222");

    server.stop();
    auto logs = get_logs();
    EXPECT_NE(logs.find("111111111111111"), std::string::npos);
    EXPECT_NE(logs.find("222222222222222"), std::string::npos);
}

TEST_F(PgwClientTest, InfoLevelFiltersOutDebugLogs) {
    auto logger = spdlog::default_logger();
    spdlog::set_level(spdlog::level::info);

    DummyUdpServer server(9987);
    server.start();

    ClientConfig config = {"127.0.0.1", 9987, "log.txt", "info"};
    ImsiClient client(config);
    client.send_imsi_request("333333333333333");

    server.stop();
    auto logs = get_logs();
    EXPECT_EQ(logs.find("[debug]"), std::string::npos);  
    EXPECT_NE(logs.find("333333333333333"), std::string::npos); 
}

TEST_F(PgwClientTest, FailsOnInvalidServerIP) {
    auto config = valid_config();
    config.server_ip = "invalid_ip";
    ImsiClient client(config, false, false, false, false);
    client.send_imsi_request("001011234567890");
    auto logs = get_logs();
    EXPECT_NE(logs.find("Invalid server IP address"), std::string::npos);
}

TEST_F(PgwClientTest, WarnsOnNoResponseAndExhaustsRetries) {
    spdlog::set_level(spdlog::level::warn);  

    ClientConfig config = {"127.0.0.1", 12345, "log.txt", "info"};
    ImsiClient client(config, false, false, false, true);
    client.send_imsi_request("001011234567890");

    auto logs = get_logs();
    EXPECT_NE(logs.find("Timeout on attempt"), std::string::npos);
    EXPECT_NE(logs.find("Max retries reached, giving up"), std::string::npos);
}
