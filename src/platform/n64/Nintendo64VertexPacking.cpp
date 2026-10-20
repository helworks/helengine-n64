#include "Nintendo64VertexPacking.hpp"

#include <cmath>

namespace helengine::n64 {

bool Nintendo64VertexPacking::WriteTextureCoordinates(T3DVertPacked& packed, uint32_t logicalIndex,
    const float uv[2], uint32_t width, uint32_t height) {
    if (uv == nullptr || width == 0 || height == 0 || !std::isfinite(uv[0]) || !std::isfinite(uv[1])) {
        return false;
    }
    // Tiny3D's importer uses uv * texture dimension * 32 and truncates toward
    // zero. Use double here so validation cannot overflow before narrowing.
    const double s = std::trunc(static_cast<double>(uv[0]) * width * 32.0);
    const double t = std::trunc(static_cast<double>(uv[1]) * height * 32.0);
    if (s < -32768.0 || s > 32767.0 || t < -32768.0 || t > 32767.0) {
        return false;
    }
    int16_t* outputUv = (logicalIndex & 1u) != 0 ? packed.stB : packed.stA;
    outputUv[0] = static_cast<int16_t>(s);
    outputUv[1] = static_cast<int16_t>(t);
    return true;
}

void Nintendo64VertexPacking::WriteVertex(T3DVertPacked& packed, uint32_t logicalIndex, const float position[3],
    float positionScale, uint16_t packedNormal, uint32_t color) {
    const bool second = (logicalIndex & 1u) != 0;
    int16_t* outputPosition = second ? packed.posB : packed.posA;
    uint16_t* outputNormal = second ? &packed.normB : &packed.normA;
    uint32_t* outputColor = second ? &packed.rgbaB : &packed.rgbaA;
    int16_t* outputUv = second ? packed.stB : packed.stA;

    outputPosition[0] = static_cast<int16_t>(std::lround(position[0] * positionScale));
    outputPosition[1] = static_cast<int16_t>(std::lround(position[1] * positionScale));
    outputPosition[2] = static_cast<int16_t>(std::lround(position[2] * positionScale));
    *outputNormal = packedNormal;
    *outputColor = color;
    outputUv[0] = 0;
    outputUv[1] = 0;
}

} // namespace helengine::n64
