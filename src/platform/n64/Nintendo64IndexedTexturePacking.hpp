#pragma once

#include <cstddef>
#include <cstdint>
#include "platform/n64/Nintendo64TexelPacking.hpp"

namespace helengine::n64 {

/// Converts the generic cooked index stream into RDP rows without expanding texels into colors.
class Nintendo64IndexedTexturePacking {
public:
    /// Validates exact compact payload lengths, supported alpha precision and every referenced palette entry.
    static bool Validate(const uint8_t* indices, size_t length, size_t pixels, bool fourBit,
                         const uint8_t* palette, size_t paletteLength, uint8_t alpha) {
        const size_t expected = fourBit ? (pixels + 1) / 2 : pixels;
        if (!indices || !palette || !pixels || length != expected || !paletteLength ||
            paletteLength % 4 || paletteLength / 4 > (fourBit ? 16u : 256u) || alpha > 3) return false;
        for (size_t i = 0; i < pixels; ++i) {
            const uint8_t index = fourBit ? ((indices[i / 2] >> ((i % 2) * 4)) & 15) : indices[i];
            if (index >= paletteLength / 4) return false;
        }
        return true;
    }

    /// Reorders LOW-first, globally packed CI4 into HIGH-first rows; unused row nibbles become zero.
    static void CopyRows(const uint8_t* source, int width, int height, bool fourBit,
                         uint8_t* destination, size_t stride) {
        for (int y = 0; y < height; ++y) {
            uint8_t* row = destination + static_cast<size_t>(y) * stride;
            for (size_t b = 0; b < stride; ++b) row[b] = 0;
            for (int x = 0; x < width; ++x) {
                const size_t pixel = static_cast<size_t>(y) * width + x;
                if (fourBit) {
                    const uint8_t index = (source[pixel / 2] >> ((pixel % 2) * 4)) & 15;
                    row[x / 2] |= index << ((x % 2 == 0) ? 4 : 0);
                } else row[x] = source[pixel];
            }
        }
    }

    /// Grayscale palettes retain independent eight-bit coverage through IA16 instead of binary RGBA5551.
    static bool IsGrayscale(const uint8_t* palette, size_t entries) {
        for (size_t i = 0; i < entries; ++i)
            if (palette[i * 4] != palette[i * 4 + 1] || palette[i * 4] != palette[i * 4 + 2]) return false;
        return true;
    }

    /// Applies the asset's alpha precision and writes native-endian TLUT words for the big-endian RDP host.
    static void ConvertPalette(const uint8_t* palette, size_t entries, uint8_t alpha, bool grayscale,
                               uint16_t* destination, size_t capacity) {
        for (size_t i = 0; i < capacity; ++i) {
            destination[i] = 0;
            if (i >= entries) continue;
            const uint8_t* color = palette + i * 4;
            uint8_t coverage = color[3];
            if (alpha == 0) coverage = 255;
            else if (alpha == 1) coverage = coverage >= 128 ? 255 : 0;
            else if (alpha == 2) coverage = (coverage >> 4) * 17;
            destination[i] = grayscale ? static_cast<uint16_t>((color[0] << 8) | coverage)
                : Nintendo64TexelPacking::PackRgba5551(color[0], color[1], color[2], coverage);
        }
    }
};

} // namespace helengine::n64
