#include "../include/server/http_api.hpp"
#include <httplib.h>
#include <spdlog/spdlog.h>

HttpServer::HttpServer(const ServerConfig& config,
                       std::atomic<bool>& running,
                       std::unordered_map<std::string, std::chrono::steady_clock::time_point>& sessions,
                       std::mutex& session_mutex,
                       int event_fd)
    : config_(config), running_(running), sessions_(sessions), session_mutex_(session_mutex), event_fd_(event_fd) {}

void HttpServer::run() {
    server_thread_ = std::thread([this]() {
        svr_.Get("/check_subscriber", [this](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_param("imsi")) {
                res.status = 400;
                res.set_content("Missing 'imsi' parameter\n", "text/plain");
                return;
            }
            
            auto imsi = req.get_param_value("imsi");

            std::string status = "not active";
            {
                std::lock_guard<std::mutex> lock(session_mutex_);
                if (sessions_.count(imsi)) {
                    status = "active";
                }
            }

            res.set_content(status + '\n', "text/plain");
            spdlog::info("/check_subscriber imsi={} → {}", imsi, status);
        });

        svr_.Post("/stop", [this](const httplib::Request&, httplib::Response& res) {
            running_ = false;

            uint64_t signal = 1;
            if (write(event_fd_, &signal, sizeof(signal)) != sizeof(signal)) {
                spdlog::error("Failed to write to event_fd");
            }

            res.set_content("stopping\n", "text/plain");
            spdlog::warn("/stop called: shutting down server");

            svr_.stop();
        });

        spdlog::info("HTTP server listening on port {}", config_.http_port);
        svr_.listen("0.0.0.0", config_.http_port);
    });
}

void HttpServer::stop() {
    svr_.stop(); 
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}
