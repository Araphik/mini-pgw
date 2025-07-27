#pragma once

#include <csignal>
#include <spdlog/spdlog.h>
#include <cstdlib>

class SignalHandler {
public:
    SignalHandler();
    static void handle(int signal);
};
