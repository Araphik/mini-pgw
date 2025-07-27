#include <gtest/gtest.h>
#include "../include/server/signal_handler.hpp"
#include <csignal>
#include <cstdlib>

TEST(SignalHandlerTest, RegistersSigintHandler) {
    SignalHandler handler;

    typedef void (*SignalHandlerFunc)(int);
    SignalHandlerFunc current_handler = std::signal(SIGINT, SIG_DFL);
    std::signal(SIGINT, SignalHandler::handle); 

    EXPECT_EQ(reinterpret_cast<void*>(current_handler),
              reinterpret_cast<void*>(&SignalHandler::handle));
}

TEST(SignalHandlerTest, HandlesUnknownSignal_NoExit) {
    ASSERT_EXIT({
        SignalHandler::handle(SIGTERM);
        std::exit(123); 
    }, ::testing::ExitedWithCode(123), "");
}

TEST(SignalHandlerTest, ExitsOnSigint) {
    ASSERT_EXIT({
        SignalHandler::handle(SIGINT);
    }, ::testing::ExitedWithCode(1), "");
}

TEST(SignalHandlerTest, MultipleRegistrationsAreIdempotent) {
    EXPECT_NO_THROW({
        SignalHandler handler1;
        SignalHandler handler2;
    });
}

TEST(SignalHandlerTest, IgnoresZeroSignal) {
    ASSERT_EXIT({
        SignalHandler::handle(0);
        std::exit(42);
    }, ::testing::ExitedWithCode(42), "");
}

TEST(SignalHandlerTest, HandlesSigusr1AsUnknown) {
    ASSERT_EXIT({
        SignalHandler::handle(SIGUSR1); 
        std::exit(99);
    }, ::testing::ExitedWithCode(99), "");
}

TEST(SignalHandlerTest, RegistersCorrectFunctionPointer) {
    SignalHandler handler;

    void (*current)(int) = std::signal(SIGINT, SIG_DFL);
    EXPECT_EQ(reinterpret_cast<void*>(current),
              reinterpret_cast<void*>(&SignalHandler::handle));

    std::signal(SIGINT, SignalHandler::handle);
}

TEST(SignalHandlerTest, HandleDoesNotThrow) {
    EXPECT_NO_THROW({
        SignalHandler::handle(SIGTERM);
    });
}

TEST(SignalHandlerTest, RegisterThenTrigger) {
    SignalHandler handler;
    ASSERT_EXIT({
        raise(SIGINT);
    }, ::testing::ExitedWithCode(1), "");
}


