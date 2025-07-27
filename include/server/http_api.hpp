#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <unistd.h>   
#include <stdint.h>  
#include <httplib.h> 
#include "../config_loader.hpp"

class HttpServer {
public:
    HttpServer(const ServerConfig& config,
               std::atomic<bool>& running,
               std::unordered_map<std::string, std::chrono::steady_clock::time_point>& sessions,
               std::mutex& session_mutex,
               int event_fd);

    void run();
    void stop();

private:
    const ServerConfig& config_;
    std::atomic<bool>& running_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point>& sessions_;
    std::mutex& session_mutex_;
    httplib::Server svr_;
    std::thread server_thread_;
    int event_fd_; 
};
