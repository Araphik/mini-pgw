cmake_minimum_required(VERSION 3.14)
project(pgw)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Enabling code coverage flags")
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage -fprofile-update=atomic -O0 -g)
    add_link_options(--coverage -fprofile-update=atomic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(unit_tests
    tests/server/test_decode_utils.cpp
    tests/server/test_cdr_writer.cpp
    tests/server/test_http_api.cpp
    tests/server/test_logger_initializer_server.cpp
    tests/server/test_signal_handler.cpp
    tests/server/test_pgw_server.cpp
    tests/client/test_bcd_encoder.cpp
    tests/client/test_logger_initializer_client.cpp
    tests/client/test_pgw_client.cpp
    tests/test_config_loader.cpp
    src/server/decode_utils.cpp
    src/server/cdr_writer.cpp
    src/server/http_api.cpp
    src/server/logger_initializer.cpp
    src/server/signal_handler.cpp
    src/server/pgw_server.cpp
    src/client/bcd_encoder.cpp
    src/client/logger_initializer.cpp
    src/client/pgw_client.cpp
    src/config_loader.cpp
)

target_include_directories(unit_tests PRIVATE include)
target_link_libraries(unit_tests
    gtest_main
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
target_compile_definitions(unit_tests PRIVATE UNIT_TEST)

include_directories(include)

add_executable(pgw_server
    src/server/main.cpp
    src/server/logger_initializer.cpp
    src/server/signal_handler.cpp
    src/server/decode_utils.cpp
    src/server/pgw_server.cpp
    src/server/cdr_writer.cpp
    src/server/http_api.cpp
    src/config_loader.cpp 
)

target_link_libraries(pgw_server
    PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
)

add_executable(pgw_client
    src/client/main.cpp
    src/client/logger_initializer.cpp
    src/client/pgw_client.cpp
    src/client/bcd_encoder.cpp
    src/config_loader.cpp
)

target_link_libraries(pgw_client
    PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

if(ENABLE_COVERAGE)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(NOT LCOV_PATH)
        message(FATAL_ERROR "lcov not found! Please install it (e.g., sudo apt install lcov)")
    endif()

    if(NOT GENHTML_PATH)
        message(FATAL_ERROR "genhtml not found! Please install it (e.g., sudo apt install lcov)")
    endif()

    add_custom_target(coverage
      COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info --ignore-errors mismatch,unused,unexecuted_block,negative
      COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/tests/*' '*/_deps/*' '*/spdlog/*' '*/nlohmann/*' '*/httplib/*' --ignore-errors mismatch,unused,negative --output-file coverage.cleaned.info
      COMMAND ${GENHTML_PATH} coverage.cleaned.info --output-directory coverage-report
      COMMENT "Generating coverage report in coverage-report/index.html"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

endif()