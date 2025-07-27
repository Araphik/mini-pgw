#pragma once

#include "config_loader.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>

class LoggerInitializer {
public:
    explicit LoggerInitializer(const ServerConfig& config);
};
