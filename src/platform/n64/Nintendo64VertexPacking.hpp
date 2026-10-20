#pragma once

#include <cstdint>

#include <t3d/t3d.h>

namespace helengine::n64 {

/// Writes logical vertices into Tiny3D's interleaved two-vertex packed format.
/// The class deliberately depends on nothing but the `T3DVertPacked` layout, so
/// the host test suite can exercise the slot selection and fixed-point rounding
/// without libdragon or the generated core.
class Nintendo64VertexPacking {
public:
    /// Writes one logical vertex into its interleaved slot: the A slot for an even
    /// `logicalIndex` and the B slot for an odd one, leaving the other slot untouched.
    /// Positions are scaled by `positionScale` and rounded to the nearest integer,
    /// UVs are zeroed, and the supplied packed normal and RGBA8 color are stored as given.
    static void WriteVertex(T3DVertPacked& packed, uint32_t logicalIndex, const float position[3],
        float positionScale, uint16_t packedNormal, uint32_t color);

    /// Converts normalized UVs to Tiny3D signed 10.5 texel coordinates, truncating
    /// toward zero like the SDK importer. Invalid dimensions, nonfinite values or
    /// signed16 overflow return false without modifying either packed vertex.
    static bool WriteTextureCoordinates(T3DVertPacked& packed, uint32_t logicalIndex, const float uv[2],
        uint32_t width, uint32_t height);
};

} // namespace helengine::n64
