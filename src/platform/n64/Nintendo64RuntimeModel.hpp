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
    void SetColor(uint32_t color);

private:
    T3DVertPacked* VerticesValue = nullptr;
    uint16_t* IndicesValue = nullptr;
    uint32_t VertexCountValue = 0;
    uint32_t IndexCountValue = 0;
};

} // namespace helengine::n64

#endif
