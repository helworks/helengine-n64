#pragma once

#include <cstdint>

namespace helengine::n64 {

/// Converts straight RGBA8 texels into the RDP's RGBA 5551 framebuffer-compatible format.
/// The class deliberately depends on nothing but `<cstdint>`, so the host test suite can
/// exercise the channel truncation and the alpha threshold without libdragon or the
/// generated core.
class Nintendo64TexelPacking {
public:
    /// Packs one straight RGBA8 texel into RGBA 5551; each color channel keeps its top five
    /// bits and alpha at or above 128 sets the single alpha bit.
    static uint16_t PackRgba5551(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

    /// Converts one row of `width` straight RGBA8 texels read from `rgba` into `width`
    /// RGBA 5551 words written to `output`; does nothing when either pointer is null or
    /// the width is not positive.
    static void ConvertRow(const uint8_t* rgba, int32_t width, uint16_t* output);
};

} // namespace helengine::n64
