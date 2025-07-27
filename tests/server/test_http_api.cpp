#include <gtest/gtest.h>
#include "server/http_api.hpp"
#include <httplib.h>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <unistd.h>

class HttpServerTest : public ::testing::Test {
protected:
    ServerConfig config_;
    std::atomic<bool> running_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> sessions_;
    std::mutex session_mutex_;
    int event_fd_ = -1;
    std::unique_ptr<HttpServer> server_;

    void SetUp() override {
        config_.http_port = 8081;
        running_ = true;
        event_fd_ = eventfd(0, EFD_NONBLOCK);
        server_ = std::make_unique<HttpServer>(config_, running_, sessions_, session_mutex_, event_fd_);
        server_->run();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        server_->stop();
        if (event_fd_ != -1) {
            close(event_fd_);
            event_fd_ = -1;
        }
    }
};

TEST_F(HttpServerTest, CheckSubscriberNotActive) {
    httplib::Client cli("localhost", config_.http_port);
    auto res = cli.Get("/check_subscriber?imsi=00000");
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "not active\n");
}

TEST_F(HttpServerTest, CheckSubscriberWithEmptyIMSI) {
    httplib::Client cli("localhost", config_.http_port);
    auto res = cli.Get("/check_subscriber?imsi=");
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "not active\n"); 
}

TEST_F(HttpServerTest, CheckSubscriberCaseSensitiveIMSI) {
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        sessions_["ABC123"] = std::chrono::steady_clock::now();
    }

    httplib::Client cli("localhost", config_.http_port);
    auto res = cli.Get("/check_subscriber?imsi=abc123");
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "not active\n");
}

TEST_F(HttpServerTest, CheckSubscriberKnownIMSI) {
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        sessions_["99999"] = std::chrono::steady_clock::now();
    }

    httplib::Client cli("localhost", config_.http_port);
    auto res = cli.Get("/check_subscriber?imsi=99999");
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "active\n");
}

TEST_F(HttpServerTest, StopEndpointShutsDownServer) {
    httplib::Client cli("localhost", config_.http_port);
    auto res = cli.Post("/stop", "", "text/plain");
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "stopping\n");

    // Сервер должен быть остановлен
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    res = cli.Get("/check_subscriber?imsi=99999");
    EXPECT_TRUE(res == nullptr || res->status != 200);
}

TEST_F(HttpServerTest, CheckSubscriberMissingParam) {
    httplib::Client cli("localhost", config_.http_port);
    auto res = cli.Get("/check_subscriber");
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 400);
}
