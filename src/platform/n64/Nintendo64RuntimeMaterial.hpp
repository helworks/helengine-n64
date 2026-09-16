#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cstdint>

#include "RuntimeMaterial.hpp"

namespace helengine::n64 {

/// Holds the cooked opaque color, culling, and lighting choice for one N64 material.
class Nintendo64RuntimeMaterial final : public ::RuntimeMaterial {
public:
    /// Creates one material from cooked color, culling, and lit flags.
    Nintendo64RuntimeMaterial(uint32_t color, bool doubleSided, bool lit);

    /// Packed RGBA8 base color written into every vertex.
    uint32_t Color() const { return ColorValue; }
    /// True when back faces are drawn.
    bool DoubleSided() const { return DoubleSidedValue; }
    /// True when Tiny3D ambient and directional lighting modulate the base color.
    bool Lit() const { return LitValue; }

private:
    /// Packed RGBA8 base color taken from the cooked material.
    uint32_t ColorValue;
    /// Cooked back-face culling choice.
    bool DoubleSidedValue;
    /// Cooked lighting choice.
    bool LitValue;
};

} // namespace helengine::n64

#endif
