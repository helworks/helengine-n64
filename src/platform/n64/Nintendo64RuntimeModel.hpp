#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cstdint>

#include <t3d/t3d.h>

#include "RuntimeModel.hpp"

namespace helengine::n64 {

/// Owns the uncached Tiny3D buffers used by one generated RuntimeModel.
class Nintendo64RuntimeModel final : public ::RuntimeModel {
public:
    Nintendo64RuntimeModel();
    ~Nintendo64RuntimeModel() override;

    bool Initialize(const ::ModelAsset* asset);
    void Dispose() override;

    T3DVertPacked* Vertices() const { return VerticesValue; }
    const uint16_t* Indices() const { return IndicesValue; }
    uint32_t VertexCount() const { return VertexCountValue; }
    uint32_t IndexCount() const { return IndexCountValue; }
    /// True when every cooked vertex received a packed normal.
    bool HasNormals() const { return HasNormalsValue; }
    /// True when the cooked asset supplies one finite normalized UV per vertex.
    bool HasTextureCoordinates() const { return HasTextureCoordinatesValue; }
    /// Updates UVs for the current material's texture size from retained raw UVs.
    /// The caller must fence pending RSP reads before this call. A false result
    /// leaves all packed vertices unchanged; missing/invalid UVs cannot be textured.
    bool SetTextureDimensions(uint32_t width, uint32_t height);
    void SetColor(uint32_t color);

private:
    T3DVertPacked* VerticesValue = nullptr;
    uint16_t* IndicesValue = nullptr;
    uint32_t VertexCountValue = 0;
    uint32_t IndexCountValue = 0;
    /// True once the cooked asset supplied one normal per vertex and all of them were packed.
    bool HasNormalsValue = false;
    bool HasTextureCoordinatesValue = false;
};

} // namespace helengine::n64

#endif
