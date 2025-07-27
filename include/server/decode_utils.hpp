#pragma once

#include <vector>
#include <string>
#include <cstdint>

class BcdDecoder {
public:
    std::string decode(const std::vector<uint8_t>& bcd) const;
};
