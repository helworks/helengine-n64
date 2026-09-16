#include "Nintendo64TexelPacking.hpp"

#include <cstddef>

namespace helengine::n64 {

/// Packs one straight RGBA8 texel into RGBA 5551; each color channel keeps its top five
/// bits and alpha at or above 128 sets the single alpha bit.
uint16_t Nintendo64TexelPacking::PackRgba5551(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
    const uint16_t packedRed = static_cast<uint16_t>((red >> 3) & 0x1Fu);
    const uint16_t packedGreen = static_cast<uint16_t>((green >> 3) & 0x1Fu);
    const uint16_t packedBlue = static_cast<uint16_t>((blue >> 3) & 0x1Fu);
    const uint16_t packedAlpha = static_cast<uint16_t>(alpha >= 128u ? 1u : 0u);
    return static_cast<uint16_t>((packedRed << 11) | (packedGreen << 6) | (packedBlue << 1) | packedAlpha);
}

/// Converts one row of `width` straight RGBA8 texels read from `rgba` into `width`
/// RGBA 5551 words written to `output`; does nothing when either pointer is null or
/// the width is not positive.
void Nintendo64TexelPacking::ConvertRow(const uint8_t* rgba, int32_t width, uint16_t* output) {
    if (rgba == nullptr || output == nullptr || width <= 0) {
        return;
    }
    for (int32_t index = 0; index < width; index++) {
        const uint8_t* texel = rgba + static_cast<size_t>(index) * 4u;
        output[index] = PackRgba5551(texel[0], texel[1], texel[2], texel[3]);
    }
}

} // namespace helengine::n64
