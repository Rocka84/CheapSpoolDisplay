#ifndef OPENPRINTTAG_H
#define OPENPRINTTAG_H

#include "../data/OpenSpool.h"
#include <vector>
#include <cstdint>
#ifndef USE_SDL2
#include <cbor.h>
#endif

class OpenPrintTagParser {
public:
    static bool parse(const std::vector<uint8_t>& payload, OpenSpoolData& data);
    static std::vector<uint8_t> generate(const OpenSpoolData& data);

private:
#ifndef USE_SDL2
    // CBOR decoding helpers
    static bool decodeMainSection(CborValue* it, OpenSpoolData& data);
    static bool decodeAuxSection(CborValue* it, OpenSpoolData& data);
#endif
};

#endif // OPENPRINTTAG_H
