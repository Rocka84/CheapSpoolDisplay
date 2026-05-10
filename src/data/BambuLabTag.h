#ifndef BAMBU_LAB_TAG_H
#define BAMBU_LAB_TAG_H

#include "OpenSpool.h"
#include <vector>
#include <cstdint>

class BambuLabTagParser {
public:
    /**
     * @brief Parses a Bambu Lab proprietary tag data buffer into OpenSpoolData.
     * 
     * @param data 1024 bytes of raw Mifare Classic 1K data.
     * @param uid 4-byte card UID.
     * @param output The output data structure.
     * @return true if parsing was successful.
     */
    static bool parse(const std::vector<uint8_t>& data, const uint8_t* uid, OpenSpoolData& output);

    /**
     * @brief Derives the 16 sector keys for a Bambu Lab tag.
     * 
     * @param uid 4-byte card UID.
     * @param salt 16-byte secret salt.
     * @param outputKeys Vector to store 16 keys (6 bytes each).
     * @return true if successful.
     */
    static bool deriveKeys(const uint8_t* uid, const uint8_t* salt, std::vector<std::vector<uint8_t>>& outputKeys);

private:
    static float extractFloatLE(const uint8_t* data);
    static uint16_t extractUint16LE(const uint8_t* data);
    static uint32_t extractUint32LE(const uint8_t* data);
    static std::string extractString(const uint8_t* data, size_t len);
};

#endif // BAMBU_LAB_TAG_H
