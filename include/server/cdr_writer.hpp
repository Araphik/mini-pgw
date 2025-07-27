#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <spdlog/spdlog.h>

// cdr_writer.hpp
class CdrWriter {
public:
    explicit CdrWriter(const std::string& cdr_file);

#ifdef TESTING
    CdrWriter(const std::string& cdr_file, std::ostream& test_stream);
#endif

    void write(const std::string& imsi, const std::string& action);

private:
    std::string cdr_file_;
#ifdef TESTING
    std::ostream* test_stream_ = nullptr;
#endif
};
