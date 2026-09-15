#pragma once

#include <cstdint>

#include <t3d/t3d.h>

namespace helengine::n64 {

class Nintendo64CubeMesh {
public:
    static constexpr uint32_t VertexCount = 24;
    static constexpr uint32_t PackedVertexCount = VertexCount / 2;
    static constexpr uint32_t IndexCount = 36;

    bool Initialize();
    void Shutdown();
    void Draw() const;
    bool ValidateGeometry() const;
    bool IsInitialized() const { return Vertices != nullptr; }

private:
    T3DVertPacked* Vertices = nullptr;
    uint8_t Indices[IndexCount]{};
};

} // namespace helengine::n64
