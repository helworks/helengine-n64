#include "Nintendo64VertexPacking.hpp"

#include <cmath>

namespace helengine::n64 {

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
