#include <gtest/gtest.h>
#include "../../../include/client/bcd_encoder.hpp"

// 1. Стандартное поведение
TEST(BcdEncoderTest, EncodesFull15DigitIMSI) {
    BcdEncoder encoder;
    std::string imsi = "123456789012345";
    auto result = encoder.encode(imsi);
    std::vector<uint8_t> expected = { 0x21, 0x43, 0x65, 0x87, 0x09, 0x21, 0x43, 0xF5 };
    EXPECT_EQ(result, expected);
}

// 2. Четная длина
TEST(BcdEncoderTest, EncodesEvenLengthIMSI) {
    BcdEncoder encoder;
    std::string imsi = "123456";
    std::vector<uint8_t> expected = { 0x21, 0x43, 0x65 };
    EXPECT_EQ(encoder.encode(imsi), expected);
}

// 3. Нечетная длина
TEST(BcdEncoderTest, EncodesOddLengthIMSI) {
    BcdEncoder encoder;
    std::string imsi = "12345";
    std::vector<uint8_t> expected = { 0x21, 0x43, 0xF5 };
    EXPECT_EQ(encoder.encode(imsi), expected);
}

// 4. Пустая строка
TEST(BcdEncoderTest, EncodesEmptyString) {
    BcdEncoder encoder;
    EXPECT_TRUE(encoder.encode("").empty());
}

// 5. Один символ
TEST(BcdEncoderTest, EncodesSingleDigit) {
    BcdEncoder encoder;
    std::vector<uint8_t> expected = { 0xF7 };
    EXPECT_EQ(encoder.encode("7"), expected);
}

// 6. Повторяющиеся цифры
TEST(BcdEncoderTest, EncodesRepeatedDigits) {
    BcdEncoder encoder;
    std::string imsi = "111111";
    std::vector<uint8_t> expected = { 0x11, 0x11, 0x11 };
    EXPECT_EQ(encoder.encode(imsi), expected);
}

// 7. Только 0 и 9 (границы допустимых цифр)
TEST(BcdEncoderTest, EncodesZerosAndNines) {
    BcdEncoder encoder;
    std::string imsi = "090909";
    std::vector<uint8_t> expected = { 0x90, 0x90, 0x90 };
    EXPECT_EQ(encoder.encode(imsi), expected);
}

// 8. Максимальная допустимая длина IMSI (15 цифр)
TEST(BcdEncoderTest, EncodesMaxLengthIMSI) {
    BcdEncoder encoder;
    std::string imsi(15, '1');  // "111111111111111"
    auto result = encoder.encode(imsi);
    ASSERT_EQ(result.size(), 8);
    EXPECT_EQ(result.back(), 0xF1);
}

// 9. 14 цифр — проверка корректности последнего байта без 0xF
TEST(BcdEncoderTest, Encodes14DigitIMSI) {
    BcdEncoder encoder;
    std::string imsi = "12345678901234";
    std::vector<uint8_t> expected = { 0x21, 0x43, 0x65, 0x87, 0x09, 0x21, 0x43 };
    EXPECT_EQ(encoder.encode(imsi), expected);
}

// 10. Невалидные символы — необработанный кейс (будет некорректный результат)
TEST(BcdEncoderTest, EncodeNonDigitCharacters_NoCheck) {
    BcdEncoder encoder;
    std::string imsi = "12A456";  // Символ 'A' вызовет неверный результат
    auto result = encoder.encode(imsi);
    // Проверка просто на длину — поведение пока не определено, если ты не валидируешь input
    EXPECT_EQ(result.size(), 3);
}
