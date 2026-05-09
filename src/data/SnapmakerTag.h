#ifndef SNAPMAKER_TAG_H
#define SNAPMAKER_TAG_H

#include "OpenSpool.h"
#include <vector>
#include <string>
#include <cstdint>

class SnapmakerTagParser {
public:
    /**
     * @brief Parses a Snapmaker proprietary tag data buffer into OpenSpoolData.
     * 
     * @param data 1024 bytes of raw Mifare Classic 1K data.
     * @param uid 4-byte card UID.
     * @param output The output data structure.
     * @return true if parsing was successful.
     */
    static bool parse(const std::vector<uint8_t>& data, const uint8_t* uid, OpenSpoolData& output);

    /**
     * @brief Derives a Mifare Classic sector key using Snapmaker's HKDF scheme.
     * 
     * @param uid 4-byte card UID.
     * @param sectorIndex 0-15.
     * @param keyType 'a' or 'b'.
     * @param outputKey 6-byte output buffer for the key.
     */
    static void deriveKey(const uint8_t* uid, uint8_t sectorIndex, char keyType, uint8_t* outputKey);

private:
    static std::string mapMainType(uint16_t typeId);
    static std::string mapSubType(uint16_t subtypeId);
};

#endif // SNAPMAKER_TAG_H
