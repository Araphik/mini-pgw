#include "../include/config_loader.hpp"
#include "../include/client/logger_initializer.hpp"
#include "../include/client/pgw_client.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>

int main(int argc, char* argv[]) 
{
    auto temp_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto temp_logger = std::make_shared<spdlog::logger>("temp_logger", temp_console_sink);
    spdlog::set_default_logger(temp_logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

    if (argc != 2) {
        spdlog::error("Usage: pgw_client <IMSI>");
        return 1;
    }

    std::string imsi = argv[1];
    ClientConfig config;

    try {
        config = load_client_config("../config/client_config.json");
    } catch (const std::exception& e) {
        spdlog::critical("Failed to load config: {}", e.what());
        return 1;
    }

    LoggerInitializer logger_init;
    logger_init.initialize(config);

    spdlog::info("Configuration loaded. Starting request...");
    
    ImsiClient client(config);
    client.send_imsi_request(imsi);
    
    return 0;
}