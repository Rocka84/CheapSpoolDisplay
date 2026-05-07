#ifndef WRITE_GUARD_H
#define WRITE_GUARD_H

#include "OpenSpool.h"
#include <string>

class WriteGuard {
public:
    static size_t estimateSize(const OpenSpoolData& data, const std::string& format);
    static void applySafeguards(OpenSpoolData& data, size_t capacity);
    static size_t getTagCapacity(const std::string& type);
};

#endif
