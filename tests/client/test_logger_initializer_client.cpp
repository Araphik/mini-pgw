#include <gtest/gtest.h>
#include <fstream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>

#include "../../../include/client/logger_initializer.hpp"
#include "../../../include/config_loader.hpp"

namespace fs = std::filesystem;

class LoggerInitializerTestClient : public ::testing::Test {
protected:
    std::string log_file = "test_client.log";

    void TearDown() override {
        if (fs::exists(log_file)) {
            fs::remove(log_file);
        }
    }

    ClientConfig makeConfig(const std::string& level = "info") {
        return ClientConfig{
            .server_ip = "127.0.0.1",
            .server_port = 1234,
            .log_file = log_file,
            .log_level = level
        };
    }
};

// 1. Проверка создания лог-файла
TEST_F(LoggerInitializerTestClient, CreatesLogFile) {
    auto config = makeConfig();
    LoggerInitializer().initialize(config);

    spdlog::info("test message");

    spdlog::shutdown(); 
    ASSERT_TRUE(fs::exists(log_file));
}

// 2. Проверка записи сообщения
TEST_F(LoggerInitializerTestClient, WritesMessageToLogFile) {
    auto config = makeConfig("info");
    LoggerInitializer().initialize(config);

    std::string message = "Logger writes this";
    spdlog::info(message);
    spdlog::shutdown();

    std::ifstream file(log_file);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find(message), std::string::npos);
}

// 3. Проверка установки уровня INFO
TEST_F(LoggerInitializerTestClient, LogsAtInfoLevel) {
    auto config = makeConfig("info");
    LoggerInitializer().initialize(config);

    EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
}

// 4. Проверка установки уровня DEBUG
TEST_F(LoggerInitializerTestClient, LogsAtDebugLevel) {
    auto config = makeConfig("debug");
    LoggerInitializer().initialize(config);

    EXPECT_EQ(spdlog::get_level(), spdlog::level::debug);
}

// 5. Проверка установки уровня WARNING
TEST_F(LoggerInitializerTestClient, LogsAtWarningLevel) {
    auto config = makeConfig("warning");
    LoggerInitializer().initialize(config);

    EXPECT_EQ(spdlog::get_level(), spdlog::level::warn);
}

// 6. Проверка установки уровня ERROR
TEST_F(LoggerInitializerTestClient, LogsAtErrorLevel) {
    auto config = makeConfig("error");
    LoggerInitializer().initialize(config);

    EXPECT_EQ(spdlog::get_level(), spdlog::level::err);
}

// 7. Повторная инициализация не вызывает исключений
TEST_F(LoggerInitializerTestClient, AllowsReinitialization) {
    auto config = makeConfig();
    EXPECT_NO_THROW(LoggerInitializer().initialize(config));
    EXPECT_NO_THROW(LoggerInitializer().initialize(config));
}

// 8. Проверка flush_on уровня
TEST_F(LoggerInitializerTestClient, FlushesOnInfoLevel) {
    auto config = makeConfig("info");
    LoggerInitializer().initialize(config);
    spdlog::info("This will be flushed");
    spdlog::shutdown();

    std::ifstream file(log_file);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("flushed"), std::string::npos);
}

// 9. Проверка паттерна (времени и уровня)
TEST_F(LoggerInitializerTestClient, LogPatternIncludesTimestampAndLevel) {
    auto config = makeConfig("info");
    LoggerInitializer().initialize(config);

    spdlog::info("pattern check");
    spdlog::shutdown();

    std::ifstream file(log_file);
    std::string line;
    std::getline(file, line);

    EXPECT_TRUE(line.find("[20") != std::string::npos); 
    EXPECT_TRUE(line.find("[info]") != std::string::npos);
}

// 10. Некорректный уровень логирования вызывает исключение
TEST_F(LoggerInitializerTestClient, InvalidLogLevelThrows) {
    auto config = makeConfig("no_such_level");
    EXPECT_THROW(LoggerInitializer().initialize(config), spdlog::spdlog_ex);
}

// 11. Проверка, что консольный sink установлен
TEST_F(LoggerInitializerTestClient, DefaultLoggerHasConsoleSink) {
    auto config = makeConfig("info");
    LoggerInitializer().initialize(config);

    auto logger = spdlog::default_logger();
    auto sinks = logger->sinks();
    bool has_console = false;
    for (const auto& s : sinks) {
        if (dynamic_cast<spdlog::sinks::stdout_color_sink_mt*>(s.get())) {
            has_console = true;
            break;
        }
    }
    EXPECT_TRUE(has_console);
}

// 12. Логгер с custom именем не перезаписывает file_sink (допустимый случай)
TEST_F(LoggerInitializerTestClient, FileSinkRemainsAfterReinit) {
    auto config = makeConfig("info");
    LoggerInitializer().initialize(config);

    spdlog::info("First init");
    spdlog::shutdown();

    LoggerInitializer().initialize(config);
    spdlog::info("Second init");
    spdlog::shutdown();

    std::ifstream file(log_file);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("First init"), std::string::npos);
    EXPECT_NE(content.find("Second init"), std::string::npos);
}

// 13. Несуществующий путь к лог-файлу вызывает исключение
TEST_F(LoggerInitializerTestClient, InvalidLogFilePathThrows) {
    ClientConfig config = {
        .server_ip = "127.0.0.1",
        .server_port = 1234,
        .log_file = "/nonexistent/path/log.txt",
        .log_level = "info"
    };
    EXPECT_THROW(LoggerInitializer().initialize(config), spdlog::spdlog_ex);
}

// 14. Сообщения ниже установленного уровня логирования не попадают в лог
TEST_F(LoggerInitializerTestClient, DoesNotLogBelowSetLevel) {
    auto config = makeConfig("warning");
    LoggerInitializer().initialize(config);

    spdlog::info("This should NOT appear in log");
    spdlog::warn("This should appear in log");
    spdlog::shutdown();

    std::ifstream file(log_file);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    EXPECT_EQ(content.find("This should NOT appear in log"), std::string::npos);
    EXPECT_NE(content.find("This should appear in log"), std::string::npos);
}

// 15. Уровень "critical" поддерживается
TEST_F(LoggerInitializerTestClient, SupportsCriticalLogLevel) {
    auto config = makeConfig("critical");
    LoggerInitializer().initialize(config);

    EXPECT_EQ(spdlog::get_level(), spdlog::level::critical);
}
