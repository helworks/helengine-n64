#pragma once

#include <libdragon.h>

namespace helengine::n64 {

class Nintendo64Video {
public:
    bool Initialize();
    bool BeginFrame();
    void Clear();
    bool EndFrame();
    void Shutdown();

private:
    surface_t* DepthSurface = nullptr;
    surface_t* CurrentFrame = nullptr;
    bool DisplayInitialized = false;
    bool RdpqInitialized = false;
};

} // namespace helengine::n64
