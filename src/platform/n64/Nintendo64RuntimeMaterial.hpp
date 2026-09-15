#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cstdint>

#include "RuntimeMaterial.hpp"

namespace helengine::n64 {

class Nintendo64RuntimeMaterial final : public ::RuntimeMaterial {
public:
    Nintendo64RuntimeMaterial(uint32_t color, bool doubleSided);

    uint32_t Color() const { return ColorValue; }
    bool DoubleSided() const { return DoubleSidedValue; }

private:
    uint32_t ColorValue;
    bool DoubleSidedValue;
};

} // namespace helengine::n64

#endif
