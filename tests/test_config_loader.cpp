#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include "../../../include/config_loader.hpp"

void write_temp_file(const std::string& filename, const std::string& content) {
    std::ofstream out(filename);
    out << content;
    out.close();
}

// ================== Server ==================

TEST(ConfigLoaderTest, LoadsValidServerConfig) {
    std::string path = "temp_server_config.json";
    write_temp_file(path, R"({
        "udp_ip": "127.0.0.1",
        "udp_port": 5000,
        "http_port": 8000,
        "log_file": "srv.log",
        "log_level": "info",
        "session_timeout_sec": 10,
        "cdr_file": "cdr.log",
        "blacklist": ["123456789012345", "987654321098765"]
    })");

    EXPECT_NO_THROW({
        ServerConfig config = load_config_server(path);
        EXPECT_EQ(config.udp_ip, "127.0.0.1");
        EXPECT_EQ(config.udp_port, 5000);
        EXPECT_EQ(config.blacklist.size(), 2);
    });

    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, InvalidUdpIpThrows) {
    std::string path = "invalid_udp_ip.json";
    write_temp_file(path, R"({ "udp_ip": "999.999.999.999" })");
    EXPECT_THROW(load_config_server(path), std::runtime_error);
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, InvalidBlacklistImsiThrows) {
    std::string path = "bad_blacklist.json";
    write_temp_file(path, R"({ "blacklist": ["notnumeric", "123"] })");
    EXPECT_THROW(load_config_server(path), std::runtime_error);
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, InvalidLogLevelThrows_Server) {
    std::string path = "bad_level_server.json";
    write_temp_file(path, R"({ "log_level": "verbose" })");
    EXPECT_THROW(load_config_server(path), std::runtime_error);
    std::remove(path.c_str());
}

// ================== Client ==================

TEST(ConfigLoaderTest, LoadsValidClientConfig) {
    std::string path = "temp_client_config.json";
    write_temp_file(path, R"({
        "server_ip": "192.168.0.1",
        "server_port": 5050,
        "log_file": "client.log",
        "log_level": "debug"
    })");

    EXPECT_NO_THROW({
        ClientConfig config = load_client_config(path);
        EXPECT_EQ(config.server_ip, "192.168.0.1");
        EXPECT_EQ(config.server_port, 5050);
        EXPECT_EQ(config.log_level, "debug");
    });

    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, InvalidServerIpThrows) {
    std::string path = "bad_server_ip.json";
    write_temp_file(path, R"({ "server_ip": "abc.def.ghi.jkl" })");
    EXPECT_THROW(load_client_config(path), std::runtime_error);
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, InvalidLogLevelThrows_Client) {
    std::string path = "bad_level_client.json";
    write_temp_file(path, R"({ "log_level": "trace" })");
    EXPECT_THROW(load_client_config(path), std::runtime_error);
    std::remove(path.c_str());
}

// ================== Validation Functions ==================

TEST(ConfigLoaderTest, ValidIpAccepted) {
    EXPECT_TRUE(is_valid_ip("192.168.1.1"));
    EXPECT_TRUE(is_valid_ip("8.8.8.8"));
    EXPECT_FALSE(is_valid_ip("999.999.999.999"));
    EXPECT_FALSE(is_valid_ip("abcd"));
}

TEST(ConfigLoaderTest, ValidLogLevelsAccepted) {
    EXPECT_TRUE(is_valid_log_level("info"));
    EXPECT_TRUE(is_valid_log_level("debug"));
    EXPECT_FALSE(is_valid_log_level("trace"));
}

TEST(ConfigLoaderTest, BlacklistValidationWorks) {
    EXPECT_NO_THROW(validate_blacklist({"123456789012345", "999999999999999"}));
    EXPECT_THROW(validate_blacklist({"short", "1234abc"}), std::runtime_error);
}

TEST(ConfigLoaderTest, UdpPortLowerBoundValid) {
    std::string path = "udp_port_min.json";
    write_temp_file(path, R"({ "udp_ip": "127.0.0.1", "udp_port": 1 })");
    EXPECT_NO_THROW(load_config_server(path));
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, UdpPortUpperBoundValid) {
    std::string path = "udp_port_max.json";
    write_temp_file(path, R"({ "udp_ip": "127.0.0.1", "udp_port": 65535 })");
    EXPECT_NO_THROW(load_config_server(path));
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, HttpPortTooLowThrows) {
    std::string path = "http_port_low.json";
    write_temp_file(path, R"({ "http_port": 0 })");
    EXPECT_THROW(load_config_server(path), std::runtime_error);
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, SessionTimeoutZeroThrows) {
    std::string path = "zero_timeout.json";
    write_temp_file(path, R"({ "session_timeout_sec": 0 })");
    EXPECT_THROW(load_config_server(path), std::runtime_error);
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, MissingOptionalFieldsUsesDefaults) {
    std::string path = "missing_fields.json";
    write_temp_file(path, R"({
        "udp_ip": "127.0.0.1",
        "udp_port": 1234,
        "http_port": 5678,
        "log_level": "info",
        "session_timeout_sec": 10,
        "blacklist": []
    })");

    ServerConfig config = load_config_server(path);
    EXPECT_EQ(config.log_file, "server.log");
    EXPECT_EQ(config.cdr_file, "cdr.log");
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, ShortImsiInBlacklistThrows) {
    std::string path = "short_imsi.json";
    write_temp_file(path, R"({
        "blacklist": ["1234567890123"]
    })");
    EXPECT_THROW(load_config_server(path), std::runtime_error);
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, NonDigitImsiInBlacklistThrows) {
    std::string path = "nondigit_imsi.json";
    write_temp_file(path, R"({
        "blacklist": ["1234567890abcde"]
    })");
    EXPECT_THROW(load_config_server(path), std::runtime_error);
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, EmptyBlacklistIsAccepted) {
    std::string path = "empty_blacklist.json";
    write_temp_file(path, R"({
        "udp_ip": "127.0.0.1",
        "udp_port": 4000,
        "http_port": 8000,
        "log_level": "info",
        "session_timeout_sec": 5,
        "blacklist": []
    })");

    ServerConfig config = load_config_server(path);
    EXPECT_TRUE(config.blacklist.empty());
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, LogFileFieldCanBeEmptyString) {
    std::string path = "empty_log_file.json";
    write_temp_file(path, R"({
        "udp_ip": "127.0.0.1",
        "udp_port": 1234,
        "http_port": 8080,
        "log_file": "",
        "log_level": "info",
        "session_timeout_sec": 30,
        "blacklist": []
    })");

    ServerConfig config = load_config_server(path);
    EXPECT_EQ(config.log_file, "");
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, ClientConfigDefaultsUsedWhenMissing) {
    std::string path = "client_missing_fields.json";
    write_temp_file(path, R"({
        "server_ip": "127.0.0.1"
    })");

    ClientConfig config = load_client_config(path);
    EXPECT_EQ(config.server_port, 9000);
    EXPECT_EQ(config.log_file, "client.log");
    EXPECT_EQ(config.log_level, "info");
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, InvalidUdpPortLowThrows) {
    std::string path = "udp_port_low.json";
    write_temp_file(path, R"({ "udp_ip": "127.0.0.1", "udp_port": 0 })");
    EXPECT_THROW(load_config_server(path), std::runtime_error);
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, InvalidUdpPortHighThrows) {
    std::string path = "udp_port_high.json";
    write_temp_file(path, R"({ "udp_ip": "127.0.0.1", "udp_port": 70000 })");
    EXPECT_THROW(load_config_server(path), std::runtime_error);
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, ClientServerPortLowThrows) {
    std::string path = "client_port_low.json";
    write_temp_file(path, R"({ "server_ip": "127.0.0.1", "server_port": 0 })");
    EXPECT_THROW(load_client_config(path), std::runtime_error);
    std::remove(path.c_str());
}

TEST(ConfigLoaderTest, ClientServerPortHighThrows) {
    std::string path = "client_port_high.json";
    write_temp_file(path, R"({ "server_ip": "127.0.0.1", "server_port": 70000 })");
    EXPECT_THROW(load_client_config(path), std::runtime_error);
    std::remove(path.c_str());
}

