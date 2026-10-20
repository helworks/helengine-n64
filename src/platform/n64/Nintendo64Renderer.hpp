#pragma once

#include <libdragon.h>
#include <t3d/t3d.h>

#include "Nintendo64CubeMesh.hpp"
#include "Nintendo64FrameLights.hpp"

namespace helengine::n64 {

class Nintendo64Renderer {
public:
    bool Initialize();
    bool InitializeAuthored();
    void BeginScene();
    void Prepare(float angleX, float angleY);
    void SetAuthoredCamera(const T3DMat4& viewMatrix, float fieldOfViewRadians, float aspectRatio, float nearPlane, float farPlane);
    /// Draws one authored indexed mesh with the supplied model matrix, applying the frame's
    /// ambient and directional lights when the material is lit and disabling Tiny3D lighting
    /// entirely when it is not. A supplied TMEM-compatible texture modulates that vertex shading;
    /// its palette must already be bound and packed UVs must use its texel dimensions.
    void DrawAuthoredMesh(const T3DVertPacked* vertices, uint32_t vertexCount, const uint16_t* indices, uint32_t indexCount, const T3DMat4& modelMatrix, bool doubleSided, bool lit, const Nintendo64FrameLights& lights, const surface_t* texture = nullptr);
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
