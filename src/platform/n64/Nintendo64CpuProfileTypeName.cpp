#include "src/platform/n64/Nintendo64CpuProfileTypeName.hpp"

#include <cstddef>

namespace helengine::n64 {

bool DecodeItaniumTypeName(const char* encoded, char (&out)[48]) {
    if (encoded == nullptr || encoded[0] == '\0') return false;

    const char* cursor = encoded;
    const bool nested = *cursor == 'N';
    if (nested) ++cursor;
    const char* lastSegment = nullptr;
    std::size_t lastLength = 0u;

    while (*cursor != '\0' && *cursor != 'E') {
        if (*cursor < '0' || *cursor > '9') return false;
        std::size_t length = 0u;
        while (*cursor >= '0' && *cursor <= '9') {
            const std::size_t digit = static_cast<std::size_t>(*cursor - '0');
            if (length > (sizeof(out) * 100u - digit) / 10u) return false;
            length = length * 10u + digit;
            ++cursor;
        }
        if (length == 0u) return false;

        const char* segment = cursor;
        std::size_t consumed = 0u;
        while (consumed < length && cursor[consumed] != '\0') ++consumed;
        if (consumed != length) return false;
        lastSegment = segment;
        lastLength = length;
        cursor += length;
    }

    if (nested && *cursor != 'E') return false;
    if (lastSegment == nullptr || lastLength == 0u || lastLength >= sizeof(out)) return false;
    for (std::size_t i = 0u; i < lastLength; ++i) out[i] = lastSegment[i];
    out[lastLength] = '\0';
    return true;
}

std::uint32_t StableTypeNameHash(const char* typeName) {
    std::uint32_t hash = 2166136261u;
    if (typeName == nullptr) return hash;
    while (*typeName != '\0') {
        hash ^= static_cast<std::uint8_t>(*typeName++);
        hash *= 16777619u;
    }
    return hash;
}

} // namespace helengine::n64
