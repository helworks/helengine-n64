#include "platform/n64/Nintendo64RuntimeModel.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cmath>

#include <libdragon.h>
#include <t3d/t3d.h>

#include "ModelAsset.hpp"
#include "RuntimeSubmesh.hpp"
#include "runtime/array.hpp"

namespace helengine::n64 {
namespace {

constexpr float kPositionScale = 64.0f;
constexpr int32_t kMaxVertices = 64;
constexpr int32_t kMaxIndices = 384; // 128 triangle-list triangles.

/// Writes one logical vertex into its interleaved Tiny3D slot with a packed normal, or a zero normal when none is supplied.
void SetPackedVertex(T3DVertPacked& packed, uint32_t logicalIndex, const float3& position, const float3* normal) {
    const bool second = (logicalIndex & 1u) != 0;
    int16_t* outputPosition = second ? packed.posB : packed.posA;
    uint16_t* outputNormal = second ? &packed.normB : &packed.normA;
    uint32_t* outputColor = second ? &packed.rgbaB : &packed.rgbaA;
    int16_t* outputUv = second ? packed.stB : packed.stA;

    outputPosition[0] = static_cast<int16_t>(std::lround(position.X * kPositionScale));
    outputPosition[1] = static_cast<int16_t>(std::lround(position.Y * kPositionScale));
    outputPosition[2] = static_cast<int16_t>(std::lround(position.Z * kPositionScale));
    if (normal == nullptr) {
        *outputNormal = 0;
    } else {
        const T3DVec3 packedNormal = {{normal->X, normal->Y, normal->Z}};
        *outputNormal = t3d_vert_pack_normal(&packedNormal);
    }
    *outputColor = 0xFFFFFFFFu;
    outputUv[0] = 0;
    outputUv[1] = 0;
}

/// True when the position is finite and fits the fixed-point vertex range once scaled.
bool IsFinitePosition(const float3& position) {
    return std::isfinite(position.X) && std::isfinite(position.Y) && std::isfinite(position.Z) &&
        std::fabs(position.X * kPositionScale) <= 32767.0f &&
        std::fabs(position.Y * kPositionScale) <= 32767.0f &&
        std::fabs(position.Z * kPositionScale) <= 32767.0f;
}

/// True when every component of a cooked normal is finite; packing has no fixed-point range to respect.
bool IsFiniteNormal(const float3& normal) {
    return std::isfinite(normal.X) && std::isfinite(normal.Y) && std::isfinite(normal.Z);
}

} // namespace

Nintendo64RuntimeModel::Nintendo64RuntimeModel() = default;

Nintendo64RuntimeModel::~Nintendo64RuntimeModel() {
    Dispose();
}

bool Nintendo64RuntimeModel::Initialize(const ::ModelAsset* asset) {
    if (asset == nullptr || asset->Positions == nullptr || asset->Indices16 == nullptr) {
        return false;
    }
    const int32_t vertexCount = asset->Positions->get_Length();
    const int32_t indexCount = asset->Indices16->get_Length();
    if (vertexCount <= 0 || vertexCount > kMaxVertices || indexCount < 3 ||
        indexCount > kMaxIndices || (indexCount % 3) != 0) {
        return false;
    }
    for (int32_t index = 0; index < indexCount; index++) {
        if ((*asset->Indices16)[index] >= static_cast<uint16_t>(vertexCount)) {
            return false;
        }
    }
    for (int32_t vertex = 0; vertex < vertexCount; vertex++) {
        if (!IsFinitePosition((*asset->Positions)[vertex])) {
            return false;
        }
    }
    const bool hasNormals = asset->Normals != nullptr && asset->Normals->get_Length() == vertexCount;
    if (asset->Normals != nullptr && asset->Normals->get_Length() != 0 && !hasNormals) {
        return false; // a partial normal array is a cooker defect, not something to guess around
    }
    if (hasNormals) {
        for (int32_t vertex = 0; vertex < vertexCount; vertex++) {
            if (!IsFiniteNormal((*asset->Normals)[vertex])) {
                return false;
            }
        }
    }

    VerticesValue = static_cast<T3DVertPacked*>(
        malloc_uncached(sizeof(T3DVertPacked) * static_cast<size_t>((vertexCount + 1) / 2)));
    IndicesValue = static_cast<uint16_t*>(
        malloc_uncached(sizeof(uint16_t) * static_cast<size_t>(indexCount)));
    if (VerticesValue == nullptr || IndicesValue == nullptr) {
        Dispose();
        return false;
    }
    const uint32_t packedCount = static_cast<uint32_t>((vertexCount + 1) / 2);
    for (uint32_t index = 0; index < packedCount; index++) {
        VerticesValue[index] = {};
    }
    for (int32_t vertex = 0; vertex < vertexCount; vertex++) {
        SetPackedVertex(VerticesValue[vertex / 2], static_cast<uint32_t>(vertex), (*asset->Positions)[vertex],
            hasNormals ? &(*asset->Normals)[vertex] : nullptr);
    }
    HasNormalsValue = hasNormals;
    for (int32_t index = 0; index < indexCount; index++) {
        IndicesValue[index] = (*asset->Indices16)[index];
    }
    VertexCountValue = static_cast<uint32_t>(vertexCount);
    IndexCountValue = static_cast<uint32_t>(indexCount);

    SetBounds(asset->BoundsMin, asset->BoundsMax);
    SetRawModelAsset(const_cast<::ModelAsset*>(asset));
    Array<RuntimeSubmesh*>* submeshes = new Array<RuntimeSubmesh*>(1);
    RuntimeSubmesh* submesh = new RuntimeSubmesh();
    submesh->IndexStart = 0;
    submesh->IndexCount = indexCount;
    submesh->PrimitiveTopology = ModelPrimitiveTopology::TriangleList;
    (*submeshes)[0] = submesh;
    SetSubmeshes(submeshes);
    delete submeshes;
    return true;
}

void Nintendo64RuntimeModel::Dispose() {
    if (VerticesValue != nullptr) {
        free_uncached(VerticesValue);
        VerticesValue = nullptr;
    }
    if (IndicesValue != nullptr) {
        free_uncached(IndicesValue);
        IndicesValue = nullptr;
    }
    VertexCountValue = 0;
    IndexCountValue = 0;
    HasNormalsValue = false;
    ::RuntimeModel::Dispose();
}

void Nintendo64RuntimeModel::SetColor(uint32_t color) {
    if (VerticesValue == nullptr) {
        return;
    }
    for (uint32_t vertex = 0; vertex < VertexCountValue; vertex++) {
        if ((vertex & 1u) == 0) {
            VerticesValue[vertex / 2].rgbaA = color;
        } else {
            VerticesValue[vertex / 2].rgbaB = color;
        }
    }
}

} // namespace helengine::n64

#endif
