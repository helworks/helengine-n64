#pragma once

#include <t3d/t3d.h>

#include "Nintendo64CubeMesh.hpp"

namespace helengine::n64 {

class Nintendo64Renderer {
public:
    bool Initialize();
    bool InitializeAuthored();
    void BeginScene();
    void Prepare(float angleX, float angleY);
    void SetAuthoredCamera(const T3DMat4& viewMatrix, float fieldOfViewRadians, float aspectRatio, float nearPlane, float farPlane);
    void DrawAuthoredMesh(const T3DVertPacked* vertices, uint32_t vertexCount, const uint16_t* indices, uint32_t indexCount, const T3DMat4& modelMatrix, uint32_t color, bool doubleSided);
    void DrawCube();
    void Shutdown();
    bool IsInitialized() const { return Initialized; }

private:
    Nintendo64CubeMesh Cube;
    T3DViewport Viewport{};
    T3DMat4FP* ModelMatrix = nullptr;
    bool Initialized = false;
    bool Authored = false;
};

} // namespace helengine::n64
