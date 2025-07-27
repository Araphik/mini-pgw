#include "../include/client/logger_initializer.hpp"

void LoggerInitializer::initialize(const ClientConfig& config) {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.log_file, false);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    std::vector<spdlog::sink_ptr> sinks{file_sink, console_sink};
    auto logger = std::make_shared<spdlog::logger>("client_logger", sinks.begin(), sinks.end());
    spdlog::set_default_logger(logger);

    auto level = spdlog::level::from_str(config.log_level);
    
    if (config.log_level != spdlog::level::to_string_view(level)) {
        throw spdlog::spdlog_ex("Invalid log level: " + config.log_level);
    }

    spdlog::set_level(level);
    logger->flush_on(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");
}
