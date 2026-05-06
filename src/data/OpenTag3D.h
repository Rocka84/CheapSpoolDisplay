#ifndef OPEN_TAG_3D_H
#define OPEN_TAG_3D_H

#include "OpenSpool.h"
#include <vector>
#include <cstdint>

class OpenTag3DParser {
public:
    static bool parseBinary(const std::vector<uint8_t>& payload, OpenSpoolData& data);

private:
    static std::string trimString(const char* data, size_t maxLen);
    static uint16_t readUint16BE(const uint8_t* data);
};

#endif // OPEN_TAG_3D_H
