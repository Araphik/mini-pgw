#include "../include/server/decode_utils.hpp"

std::string BcdDecoder::decode(const std::vector<uint8_t>& bcd) const {
    std::string imsi;
    for (auto byte : bcd) {
        uint8_t low = byte & 0x0F;
        uint8_t high = (byte >> 4) & 0x0F;
        if (low < 10) imsi += ('0' + low);
        if (high < 10) imsi += ('0' + high);
    }
    return imsi;}
