#ifndef OPEN_TAG_3D_H
#define OPEN_TAG_3D_H

#include "OpenSpool.h"
#include <vector>
#include <cstdint>

class OpenTag3DParser {
public:
    static bool parseBinary(const std::vector<uint8_t>& payload, OpenSpoolData& data);
    static std::vector<uint8_t> generateBinary(const OpenSpoolData& data);

private:
    static std::string trimString(const char* data, size_t maxLen);
    static uint16_t readUint16BE(const uint8_t* data);
    static void writeUint16BE(uint8_t* data, uint16_t value);
};

#endif // OPEN_TAG_3D_H
