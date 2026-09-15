#include "Nintendo64CubeMesh.hpp"

#include <libdragon.h>

namespace helengine::n64 {
namespace {

constexpr int16_t CubePositions[Nintendo64CubeMesh::VertexCount][3] = {
    // Front (-Z)
    {-2, -2, -2}, {-2,  2, -2}, { 2,  2, -2}, { 2, -2, -2},
    // Back (+Z)
    {-2, -2,  2}, { 2, -2,  2}, { 2,  2,  2}, {-2,  2,  2},
    // Left (-X)
    {-2, -2,  2}, {-2,  2,  2}, {-2,  2, -2}, {-2, -2, -2},
    // Right (+X)
    { 2, -2, -2}, { 2,  2, -2}, { 2,  2,  2}, { 2, -2,  2},
    // Top (+Y)
    {-2,  2, -2}, {-2,  2,  2}, { 2,  2,  2}, { 2,  2, -2},
    // Bottom (-Y)
    {-2, -2, -2}, { 2, -2, -2}, { 2, -2,  2}, {-2, -2,  2},
};

constexpr uint32_t FaceColors[6] = {
    0xE84A4AFF, // front
    0x4A90E2FF, // back
    0x55B76BFF, // left
    0xF2B84BFF, // right
    0xB16BCEFF, // top
    0xE8874AFF, // bottom
};

constexpr int16_t ExpectedNormals[6][3] = {
    { 0,  0, -1},
    { 0,  0,  1},
    {-1,  0,  0},
    { 1,  0,  0},
    { 0,  1,  0},
    { 0, -1,  0},
};

constexpr uint8_t CubeIndices[Nintendo64CubeMesh::IndexCount] = {
     0,  1,  2,  0,  2,  3,
     4,  5,  6,  4,  6,  7,
     8,  9, 10,  8, 10, 11,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 21, 22, 20, 22, 23,
};

void SetVertex(T3DVertPacked& packed, uint32_t logicalIndex,
               const int16_t position[3], uint32_t color) {
    const bool second = (logicalIndex & 1u) != 0;
    int16_t* outPosition = second ? packed.posB : packed.posA;
    uint16_t* outNormal = second ? &packed.normB : &packed.normA;
    uint32_t* outColor = second ? &packed.rgbaB : &packed.rgbaA;
    int16_t* outUv = second ? packed.stB : packed.stA;

    outPosition[0] = position[0];
    outPosition[1] = position[1];
    outPosition[2] = position[2];
    *outNormal = 0;
    *outColor = color;
    outUv[0] = 0;
    outUv[1] = 0;
}

} // namespace

bool Nintendo64CubeMesh::Initialize() {
    if (Vertices != nullptr) {
        return true;
    }

    Vertices = static_cast<T3DVertPacked*>(
        malloc_uncached(sizeof(T3DVertPacked) * PackedVertexCount));
    if (Vertices == nullptr) {
        debugf("n64:error cube-vertex-allocation-failed\n");
        return false;
    }

    for (uint32_t i = 0; i < PackedVertexCount; ++i) {
        Vertices[i] = {};
    }
    for (uint32_t i = 0; i < VertexCount; ++i) {
        SetVertex(Vertices[i / 2], i, CubePositions[i], FaceColors[i / 4]);
    }
    for (uint32_t i = 0; i < IndexCount; ++i) {
        Indices[i] = CubeIndices[i];
    }

    if (!ValidateGeometry()) {
        debugf("n64:error cube-geometry-validation-failed\n");
        Shutdown();
        return false;
    }
    return true;
}

void Nintendo64CubeMesh::Shutdown() {
    if (Vertices != nullptr) {
        free_uncached(Vertices);
        Vertices = nullptr;
    }
}

bool Nintendo64CubeMesh::ValidateGeometry() const {
    if (Vertices == nullptr) {
        return false;
    }

    // Validate winding against each face's expected outward normal. This
    // protects the explicit back-face culling state from a silent mesh error.
    for (uint32_t face = 0; face < 6; ++face) {
        const uint8_t a = Indices[face * 6 + 0];
        const uint8_t b = Indices[face * 6 + 1];
        const uint8_t c = Indices[face * 6 + 2];

        const int32_t ab[3] = {
            CubePositions[b][0] - CubePositions[a][0],
            CubePositions[b][1] - CubePositions[a][1],
            CubePositions[b][2] - CubePositions[a][2],
        };
        const int32_t ac[3] = {
            CubePositions[c][0] - CubePositions[a][0],
            CubePositions[c][1] - CubePositions[a][1],
            CubePositions[c][2] - CubePositions[a][2],
        };
        const int32_t cross[3] = {
            ab[1] * ac[2] - ab[2] * ac[1],
            ab[2] * ac[0] - ab[0] * ac[2],
            ab[0] * ac[1] - ab[1] * ac[0],
        };
        const int32_t dot = cross[0] * ExpectedNormals[face][0] +
                            cross[1] * ExpectedNormals[face][1] +
                            cross[2] * ExpectedNormals[face][2];
        if (dot <= 0) {
            return false;
        }
    }
    return true;
}

void Nintendo64CubeMesh::Draw() const {
    if (Vertices == nullptr) {
        return;
    }

    t3d_vert_load(Vertices, 0, VertexCount);
    for (uint32_t i = 0; i < IndexCount; i += 3) {
        t3d_tri_draw(Indices[i + 0], Indices[i + 1], Indices[i + 2]);
    }
}

} // namespace helengine::n64
