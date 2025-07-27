#include "../include/config_loader.hpp"
#include <regex>
#include <stdexcept>
#include <sstream>

bool is_valid_ip(const std::string& ip) {
    std::regex ip_regex(R"(^((25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.|$)){4}$)");
    return std::regex_match(ip, ip_regex);
}

bool is_valid_log_level(const std::string& level) {
    return level == "info" || level == "debug" || level == "warning" || level == "error";
}

void validate_blacklist(const std::vector<std::string>& blacklist) {
    for (const auto& imsi : blacklist) {
        if (imsi.size() != 15 || !std::all_of(imsi.begin(), imsi.end(), ::isdigit)) {
            std::ostringstream oss;
            oss << "Invalid IMSI in blacklist: " << imsi << ". IMSI must be exactly 15 digits.";
            throw std::runtime_error(oss.str());
        }
    }
}

ServerConfig load_config_server(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) throw std::runtime_error("Failed to open server config file: " + filepath);

    nlohmann::json j;
    file >> j;

    ServerConfig config;
    config.udp_ip = j.value("udp_ip", "0.0.0.0");
    if (!is_valid_ip(config.udp_ip)) {
        throw std::runtime_error("Invalid udp_ip: " + config.udp_ip);
    }

    config.udp_port = j.value("udp_port", 9000);
    if (config.udp_port <= 0 || config.udp_port > 65535) {
        throw std::runtime_error("Invalid udp_port: must be 1-65535");
    }

    config.http_port = j.value("http_port", 8080);
    if (config.http_port <= 0 || config.http_port > 65535) {
        throw std::runtime_error("Invalid http_port: must be 1-65535");
    }

    config.log_file = j.value("log_file", "server.log");
    config.log_level = j.value("log_level", "info");
    if (!is_valid_log_level(config.log_level)) {
        throw std::runtime_error("Invalid log_level: " + config.log_level);
    }

    config.session_timeout_sec = j.value("session_timeout_sec", 30);
    if (config.session_timeout_sec <= 0) {
        throw std::runtime_error("Invalid session_timeout_sec: must be > 0");
    }

    config.cdr_file = j.value("cdr_file", "cdr.log");

    config.blacklist = j.value("blacklist", std::vector<std::string>{});
    validate_blacklist(config.blacklist);

    return config;
}

ClientConfig load_client_config(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) throw std::runtime_error("Failed to open client config file: " + filepath);

    nlohmann::json j;
    file >> j;

    ClientConfig config;
    config.server_ip = j.value("server_ip", "127.0.0.1");
    if (!is_valid_ip(config.server_ip)) {
        throw std::runtime_error("Invalid server_ip: " + config.server_ip);
    }

    config.server_port = j.value("server_port", 9000);
    if (config.server_port <= 0 || config.server_port > 65535) {
        throw std::runtime_error("Invalid server_port: must be 1-65535");
    }

    config.log_file = j.value("log_file", "client.log");
    config.log_level = j.value("log_level", "info");
    if (!is_valid_log_level(config.log_level)) {
        throw std::runtime_error("Invalid log_level: " + config.log_level);
    }

    return config;
}
