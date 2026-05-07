#ifndef OPENPRINTTAG_H
#define OPENPRINTTAG_H

#include "../data/OpenSpool.h"
#include <vector>
#include <cstdint>
#include "cbor.h"

class OpenPrintTagParser {
public:
    static bool parse(const std::vector<uint8_t>& payload, OpenSpoolData& spoolData);
    static std::vector<uint8_t> generate(const OpenSpoolData& spoolData);

private:
    // CBOR decoding helpers
    static bool decodeMainSection(CborValue* it, OpenSpoolData& spoolData);
    static bool decodeAuxSection(CborValue* it, OpenSpoolData& spoolData);
};

#endif // OPENPRINTTAG_H
