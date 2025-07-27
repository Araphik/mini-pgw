#include "../include/server/cdr_writer.hpp"

// cdr_writer.cpp
CdrWriter::CdrWriter(const std::string& cdr_file) : cdr_file_(cdr_file) {}

#ifdef TESTING
CdrWriter::CdrWriter(const std::string& cdr_file, std::ostream& test_stream)
    : cdr_file_(cdr_file), test_stream_(&test_stream) {}
#endif

void CdrWriter::write(const std::string& imsi, const std::string& action) {
#ifdef TESTING
    std::ostream* out = test_stream_;
    std::ofstream file;
    if (!out) {
        file.open(cdr_file_, std::ios::app);
        out = &file;
    }
#else
    std::ofstream file(cdr_file_, std::ios::app);
    std::ostream* out = &file;
#endif

    if (!(*out)) {
        spdlog::critical("Unable to open CDR file: {}", cdr_file_);
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");

    *out << oss.str() << ", " << imsi << ", " << action << "\n";

    if (!(*out)) {
        spdlog::error("Failed to write CDR record for IMSI {} to file {}", imsi, cdr_file_);
    } else {
        spdlog::debug("CDR record written: {} | {} | {}", oss.str(), imsi, action);
    }
}

