#pragma once
#include "../include/config_loader.hpp"
#include "../include/client/bcd_encoder.hpp"
#include <spdlog/spdlog.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>

class ImsiClient {
public:
    ImsiClient(const ClientConfig& config,
               bool fail_socket = false,
               bool fail_sendto = false,
               bool fail_select = false,
               bool fail_recv = false);
    ~ImsiClient() = default;
    const int MAX_RETRIES = 5;

    void send_imsi_request(const std::string& imsi);

private:
    ClientConfig config_;
    bool fail_socket_;
    bool fail_sendto_;
    bool fail_select_;
    bool fail_recv_;
};
