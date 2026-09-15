#pragma once

#include <t3d/t3d.h>

#include "Nintendo64CubeMesh.hpp"

namespace helengine::n64 {

class Nintendo64Renderer {
public:
    bool Initialize();
    void BeginScene();
    void Prepare(float angleX, float angleY);
    void DrawCube();
    void Shutdown();
    bool IsInitialized() const { return Initialized; }

private:
    Nintendo64CubeMesh Cube;
    T3DViewport Viewport{};
    T3DMat4FP* ModelMatrix = nullptr;
    bool Initialized = false;
};

} // namespace helengine::n64
