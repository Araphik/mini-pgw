#include "../include/config_loader.hpp"
#include "../include/server/logger_initializer.hpp"
#include "../include/server/signal_handler.hpp"
#include "../include/server/pgw_server.hpp"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

volatile sig_atomic_t g_shutdown = 0;

int main() {
    spdlog::debug("Setting up signal handler for SIGINT...");
    SignalHandler sig_handler;

    spdlog::info("PGW Server starting...");

    ServerConfig config;
    try {
        spdlog::debug("Loading configuration from ../config/server_config.json...");
        config = load_config_server("../config/server_config.json");
        spdlog::info("Configuration loaded: udp_ip={}, udp_port={}, log_level={}",
                     config.udp_ip, config.udp_port, config.log_level);
    } catch (const std::exception& e) {
        spdlog::critical("Failed to load configuration: {}", e.what());
        return 1;
    }

    try {
        spdlog::debug("Initializing logger...");
        LoggerInitializer logger(config);
    } catch (const std::exception& e) {
        spdlog::critical("{}", e.what());
        return 1;
    }

    spdlog::info("Starting UDP server...");
    try {
        PgwServer server(config);
        server.run();
        spdlog::info("UDP server stopped normally.");
    } catch (const std::exception& e) {
        spdlog::critical("UDP server crashed: {}", e.what());
        return 1;
    }

    spdlog::info("PGW Server shutdown complete.");
    return 0;
}
