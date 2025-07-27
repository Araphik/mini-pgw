#include <gtest/gtest.h>
#include "server/decode_utils.hpp"

TEST(BcdDecoderTest, ValidIMSI) {
    BcdDecoder decoder;
    std::vector<uint8_t> bcd = {0x21, 0x43, 0x65, 0x87};
    EXPECT_EQ(decoder.decode(bcd), "12345678");
}

TEST(BcdDecoderTest, IgnoreInvalidDigits) {
    BcdDecoder decoder;
    std::vector<uint8_t> bcd = {0x2F, 0x4A}; 
    EXPECT_EQ(decoder.decode(bcd), "24");
}

TEST(BcdDecoderTest, EmptyVectorReturnsEmptyString) {
    BcdDecoder decoder;
    std::vector<uint8_t> bcd = {};
    EXPECT_EQ(decoder.decode(bcd), "");
}

TEST(BcdDecoderTest, AllInvalidDigits) {
    BcdDecoder decoder;
    std::vector<uint8_t> bcd = {0xFA, 0xFE}; 
    EXPECT_EQ(decoder.decode(bcd), "");
}

TEST(BcdDecoderTest, OddDigitsOnly) {
    BcdDecoder decoder;
    std::vector<uint8_t> bcd = {0x13, 0x57}; 
    EXPECT_EQ(decoder.decode(bcd), "3175");
}

TEST(BcdDecoderTest, EvenDigitsOnly) {
    BcdDecoder decoder;
    std::vector<uint8_t> bcd = {0x20, 0x64}; 
    EXPECT_EQ(decoder.decode(bcd), "0246");
}

TEST(BcdDecoderTest, HighNibbleOnlyValid) {
    BcdDecoder decoder;
    std::vector<uint8_t> bcd = {0xF1, 0xF2};
    EXPECT_EQ(decoder.decode(bcd), "12");
}

TEST(BcdDecoderTest, LowNibbleOnlyValid) {
    BcdDecoder decoder;
    std::vector<uint8_t> bcd = {0x1F, 0x2F};
    EXPECT_EQ(decoder.decode(bcd), "12");
}

TEST(BcdDecoderTest, LeadingZeroPreserved) {
    BcdDecoder decoder;
    std::vector<uint8_t> bcd = {0x10, 0x02};
    EXPECT_EQ(decoder.decode(bcd), "0120");
}

TEST(BcdDecoderTest, LongIMSI) {
    BcdDecoder decoder;
    std::vector<uint8_t> bcd = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56}; 
    EXPECT_EQ(decoder.decode(bcd), "2143658709214365");
}
