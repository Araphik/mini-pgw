#include "../include/server/logger_initializer.hpp"

LoggerInitializer::LoggerInitializer(const ServerConfig& config) {
    try {
        spdlog::debug("Creating logs directory...");
        std::filesystem::create_directory("../logs");

        spdlog::debug("Creating file sink for logger...");
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.log_file, false);

        auto logger = spdlog::default_logger();
        logger->sinks().push_back(file_sink);

        spdlog::debug("Setting log level from config: {}", config.log_level);
        auto level = spdlog::level::from_str(config.log_level);
        if (level == spdlog::level::off && config.log_level != "off") {
            spdlog::error("Invalid log level '{}', defaulting to 'info'", config.log_level);
            level = spdlog::level::info;
        }

        spdlog::set_level(level);

        spdlog::info("Logger initialized successfully with level '{}'", config.log_level);
    } catch (const std::exception& ex) {
        spdlog::critical("Logger initialization failed: {}", ex.what());
        throw std::runtime_error(std::string("Logger initialization failed: ") + ex.what());
    }
}
