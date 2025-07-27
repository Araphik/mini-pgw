#include <gtest/gtest.h>
#include "../include/config_loader.hpp"
#include "../include/server/logger_initializer.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>

class LoggerInitializerTestServer : public ::testing::Test {
protected:
    std::string test_log = "test_logger.log";

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(test_log, ec);
    }
};

TEST_F(LoggerInitializerTestServer, CreatesLogFile) {
    ServerConfig config{.log_file = test_log, .log_level = "info"};
    LoggerInitializer logger(config);
    EXPECT_TRUE(std::filesystem::exists(test_log));
}

TEST_F(LoggerInitializerTestServer, CreatesLogsDirectory) {
    std::string logs_dir = "../logs";
    std::filesystem::remove_all(logs_dir);
    ServerConfig config{.log_file = "../logs/test.log", .log_level = "debug"};
    LoggerInitializer logger(config);
    EXPECT_TRUE(std::filesystem::exists(logs_dir));
}

TEST_F(LoggerInitializerTestServer, SetsValidLogLevel) {
    ServerConfig config{.log_file = test_log, .log_level = "warn"};
    LoggerInitializer logger(config);
    EXPECT_EQ(spdlog::get_level(), spdlog::level::warn);
}

TEST_F(LoggerInitializerTestServer, DefaultsToInfoOnInvalidLevel) {
    ServerConfig config{.log_file = test_log, .log_level = "not_a_level"};
    LoggerInitializer logger(config);
    EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
}

TEST_F(LoggerInitializerTestServer, ThrowsIfFileIsUnwritable) {
    auto throwing_call = []() {
        ServerConfig config{.log_file = "/root/forbidden.log", .log_level = "info"};
        LoggerInitializer logger(config);
    };
    EXPECT_THROW(throwing_call(), std::runtime_error);
}

TEST_F(LoggerInitializerTestServer, HandlesOffLogLevel) {
    ServerConfig config{.log_file = test_log, .log_level = "off"};
    LoggerInitializer logger(config);
    EXPECT_EQ(spdlog::get_level(), spdlog::level::off);
}

TEST_F(LoggerInitializerTestServer, HandlesEmptyLogFilePath) {
    auto throwing_call = []() {
        ServerConfig config{.log_file = "", .log_level = "info"};
        LoggerInitializer logger(config);
    };
    EXPECT_THROW(throwing_call(), std::runtime_error);
}

TEST_F(LoggerInitializerTestServer, SupportsTraceLogLevel) {
    ServerConfig config{.log_file = test_log, .log_level = "trace"};
    LoggerInitializer logger(config);
    EXPECT_EQ(spdlog::get_level(), spdlog::level::trace);
}

TEST_F(LoggerInitializerTestServer, DoesNotThrowIfLogsDirectoryExists) {
    std::filesystem::create_directory("../logs");
    ServerConfig config{.log_file = "../logs/test2.log", .log_level = "info"};
    EXPECT_NO_THROW(LoggerInitializer logger(config));
    std::filesystem::remove("../logs/test2.log");
}

TEST_F(LoggerInitializerTestServer, MultipleInitializationsAppendSink) {
    ServerConfig config{.log_file = test_log, .log_level = "info"};
    LoggerInitializer logger1(config);
    EXPECT_NO_THROW(LoggerInitializer logger2(config));
}




