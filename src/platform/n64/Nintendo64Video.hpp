#pragma once

#include <libdragon.h>

namespace helengine::n64 {

#if HELENGINE_N64_CPU_PROFILE
class Nintendo64CpuProfileSink;
#endif

class Nintendo64Video {
public:
    bool Initialize();
    bool BeginFrame();
    void SetClearColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255);
    void Clear();
    /// Clears only the requested attachments; an overlay with disabled clears preserves preceding cameras.
    void ClearAttachments(bool colorEnabled, bool depthEnabled, float depth = 1.0f);
    bool EndFrame();
#if HELENGINE_N64_CPU_PROFILE
    void SetCpuProfileSink(Nintendo64CpuProfileSink* sink) { CpuProfile = sink; }
#endif
    void Shutdown();

private:
    surface_t* DepthSurface = nullptr;
    surface_t* CurrentFrame = nullptr;
    uint8_t ClearRed = 16;
    uint8_t ClearGreen = 18;
    uint8_t ClearBlue = 24;
    uint8_t ClearAlpha = 255;
    bool DisplayInitialized = false;
    bool RdpqInitialized = false;
#if HELENGINE_N64_CPU_PROFILE
    Nintendo64CpuProfileSink* CpuProfile = nullptr;
#endif
};

} // namespace helengine::n64
