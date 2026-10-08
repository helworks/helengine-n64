#pragma once

#include <cstdint>

namespace helengine::n64 {

/// Extracts the final class-name segment from GCC/Itanium type_info::name().
bool DecodeItaniumTypeName(const char* encoded, char (&out)[48]);
std::uint32_t StableTypeNameHash(const char* typeName);

} // namespace helengine::n64
