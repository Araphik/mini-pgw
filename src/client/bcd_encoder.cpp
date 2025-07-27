#include "../include/client/bcd_encoder.hpp"

std::vector<uint8_t> BcdEncoder::encode(const std::string& imsi) const {
    std::vector<uint8_t> bcd;
    for (size_t i = 0; i < imsi.length(); i += 2) {
        uint8_t byte = (imsi[i] - '0') & 0x0F;
        if (i + 1 < imsi.length()) {
            byte |= ((imsi[i + 1] - '0') & 0x0F) << 4;
        } else {
            byte |= 0xF0;
        }
        bcd.push_back(byte);
    }
    return bcd;}
