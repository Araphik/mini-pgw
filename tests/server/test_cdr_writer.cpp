#include <gtest/gtest.h>
#include "server/cdr_writer.hpp"
#include <fstream>
#include <regex>

TEST(CdrWriterTest, WriteCDRFile) {
    std::string filename = "test_cdr.log";
    std::remove(filename.c_str());

    CdrWriter writer(filename);
    writer.write("123456789012345", "create");

    std::ifstream file(filename);
    ASSERT_TRUE(file.is_open());

    std::string line;
    std::getline(file, line);
    EXPECT_NE(line.find("123456789012345"), std::string::npos);
    EXPECT_NE(line.find("create"), std::string::npos);
    file.close();

    std::remove(filename.c_str());
}

TEST(CdrWriterTest, WriteEmptyValues) {
    std::string filename = "test_cdr_empty.log";
    std::remove(filename.c_str());

    CdrWriter writer(filename);
    writer.write("", "");

    std::ifstream file(filename);
    ASSERT_TRUE(file.is_open());

    std::string line;
    std::getline(file, line);
    EXPECT_NE(line.find(", , "), std::string::npos);
    file.close();

    std::remove(filename.c_str());
}

TEST(CdrWriterTest, HandleFileOpenFailure) {
    std::string bad_path = "/nonexistent_dir/test_cdr.log";
    CdrWriter writer(bad_path);

    EXPECT_NO_THROW({
        writer.write("123456789012345", "create");
    });
}

TEST(CdrWriterTest, MultipleWritesAppend) {
    std::string filename = "test_cdr_append.log";
    std::remove(filename.c_str());

    CdrWriter writer(filename);
    writer.write("IMSI1", "create");
    writer.write("IMSI2", "timeout");

    std::ifstream file(filename);
    ASSERT_TRUE(file.is_open());

    std::string line1, line2;
    std::getline(file, line1);
    std::getline(file, line2);

    EXPECT_NE(line1.find("IMSI1"), std::string::npos);
    EXPECT_NE(line1.find("create"), std::string::npos);

    EXPECT_NE(line2.find("IMSI2"), std::string::npos);
    EXPECT_NE(line2.find("timeout"), std::string::npos);

    file.close();
    std::remove(filename.c_str());
}

TEST(CdrWriterTest, TimestampFormatCheck) {
    std::string filename = "test_cdr_time.log";
    std::remove(filename.c_str());

    CdrWriter writer(filename);
    writer.write("999", "test");

    std::ifstream file(filename);
    ASSERT_TRUE(file.is_open());

    std::string line;
    std::getline(file, line);

    std::regex ts_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}, )");
    EXPECT_TRUE(std::regex_search(line, ts_regex));

    file.close();
    std::remove(filename.c_str());
}

TEST(CdrWriterTest, WriteFailureAfterOpen) {
    std::string filename = "readonly_cdr.log";

    // Создаём файл и ставим права только на чтение
    std::ofstream init(filename);
    init << "Initial\n";
    init.close();
    chmod(filename.c_str(), 0444);  // Только чтение

    CdrWriter writer(filename);

    // Здесь запись должна упасть
    writer.write("FAIL", "write");

    chmod(filename.c_str(), 0644);  // Вернуть права, чтобы можно было удалить
    std::remove(filename.c_str());
}
