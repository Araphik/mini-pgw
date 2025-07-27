#include "../include/server/signal_handler.hpp"

SignalHandler::SignalHandler() {
    std::signal(SIGINT, SignalHandler::handle);
}

void SignalHandler::handle(int signal) {
    spdlog::debug("Signal handler triggered with signal {}", signal);
    if (signal == SIGINT) {
        spdlog::info("Shutdown signal received (Ctrl+C)");
        spdlog::shutdown();
        std::exit(1);
       
    } else {
        spdlog::warn("Unhandled signal received: {}", signal);
    }
}
