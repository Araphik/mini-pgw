#ifndef BCD_ENCODER_HPP
#define BCD_ENCODER_HPP

#include <vector>
#include <string>
#include <cstdint>

class BcdEncoder {
public:
    BcdEncoder() = default;

    std::vector<uint8_t> encode(const std::string& imsi) const;
};

#endif
