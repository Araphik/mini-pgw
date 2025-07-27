#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>

struct ServerConfig {
    std::string udp_ip;
    int udp_port;
    int http_port;
    std::string log_file;
    std::string log_level;
    int session_timeout_sec;
    std::string cdr_file;
    std::vector<std::string> blacklist;
};

struct ClientConfig {
    std::string server_ip;
    int server_port;
    std::string log_file;
    std::string log_level;
};

ServerConfig load_config_server(const std::string& filepath);
ClientConfig load_client_config(const std::string& filepath);

void send_imsi_request(const ClientConfig& config, const std::string& imsi);
void run_udp_server(const ServerConfig& config);

bool is_valid_ip(const std::string& ip);
bool is_valid_log_level(const std::string& level);
void validate_blacklist(const std::vector<std::string>& blacklist);
