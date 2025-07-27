#ifndef LOGGER_INITIALIZER_HPP
#define LOGGER_INITIALIZER_HPP

#include "../include/config_loader.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class LoggerInitializer {
public:
    LoggerInitializer() = default;
    void initialize(const ClientConfig& config);
};

#endif 
