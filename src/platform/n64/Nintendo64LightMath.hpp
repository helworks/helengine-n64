#pragma once

#include <cstdint>

namespace helengine::n64 {

/// Converts authored light colors and directions into the byte colors and unit
/// vectors Tiny3D expects.  The class is deliberately free of Tiny3D and
/// generated-core dependencies so it can be exercised by the host test suite.
class Nintendo64LightMath {
public:
    /// Scales an RGB color by intensity, clamps each channel to [0,1], and packs it as RGBA8 with alpha 255.
    static void PackColor(float red, float green, float blue, float intensity, uint8_t output[4]);
    /// Adds an intensity-scaled color into a running RGB accumulator (no clamping until PackAccumulated).
    static void Accumulate(float red, float green, float blue, float intensity, float accumulator[3]);
    /// Clamps an accumulated RGB triple to [0,1] and packs it as RGBA8 with alpha 255.
    static void PackAccumulated(const float accumulator[3], uint8_t output[4]);
    /// Converts an entity forward direction into Tiny3D's surface-to-light unit vector (negated and normalized).
    /// Returns false when the input has zero length.
    static bool ToLightVector(float forwardX, float forwardY, float forwardZ, float output[3]);
};

} // namespace helengine::n64
