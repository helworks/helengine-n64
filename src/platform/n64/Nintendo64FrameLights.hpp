#pragma once

#include <cstdint>

namespace helengine::n64 {

/// Maximum directional lights Tiny3D can evaluate in one draw.
constexpr int kMaxDirectionalLights = 7;

/// One directional light converted to Tiny3D's 8-bit color and surface-to-light unit vector.
struct Nintendo64DirectionalLight {
    /// Intensity-scaled RGBA8 light color; alpha is always 255.
    uint8_t Color[4];
    /// Unit-length surface-to-light direction, the negated entity forward axis.
    float Direction[3];
};

/// Ambient color plus up to seven directional lights for one rendered frame.
struct Nintendo64FrameLights {
    /// Summed and clamped RGBA8 ambient color; alpha is always 255.
    uint8_t Ambient[4] = {0, 0, 0, 255};
    /// Directional lights in registration order; only the first DirectionalCount entries are valid.
    Nintendo64DirectionalLight Directional[kMaxDirectionalLights] = {};
    /// Number of populated entries in Directional.
    int DirectionalCount = 0;
};

} // namespace helengine::n64
