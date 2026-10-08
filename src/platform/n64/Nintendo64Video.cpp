#include "Nintendo64Video.hpp"

#if HELENGINE_N64_CPU_PROFILE
#include "platform/n64/Nintendo64CpuProfileSink.hpp"
#endif

namespace helengine::n64 {

bool Nintendo64Video::Initialize() {
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2,
                 GAMMA_NONE, FILTERS_RESAMPLE);
    DisplayInitialized = true;

    rdpq_init();
    RdpqInitialized = true;

    DepthSurface = display_get_zbuf();
    if (DepthSurface == nullptr) {
        debugf("n64:error depth-surface-allocation-failed\n");
        Shutdown();
        return false;
    }

#if defined(HELENGINE_N64_DIAGNOSTIC)
    rdpq_debug_start();
#endif
    return true;
}

bool Nintendo64Video::BeginFrame() {
    if (DepthSurface == nullptr || CurrentFrame != nullptr) {
        debugf("n64:error invalid-frame-state\n");
        return false;
    }

    CurrentFrame = display_get();
    if (CurrentFrame == nullptr) {
        debugf("n64:error display-surface-acquisition-failed\n");
        return false;
    }

    rdpq_attach(CurrentFrame, DepthSurface);
    return true;
}

void Nintendo64Video::SetClearColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
    ClearRed = red;
    ClearGreen = green;
    ClearBlue = blue;
    ClearAlpha = alpha;
}

void Nintendo64Video::Clear() {
    rdpq_clear(RGBA32(ClearRed, ClearGreen, ClearBlue, ClearAlpha));
    rdpq_clear_z(ZBUF_MAX);
}

bool Nintendo64Video::EndFrame() {
    if (CurrentFrame == nullptr) {
        debugf("n64:error end-frame-without-frame\n");
        return false;
    }

    // This explicit wait is the ownership boundary for mutable matrices and
    // geometry introduced by the renderer in later checkpoints.
#if HELENGINE_N64_CPU_PROFILE
    const std::uint64_t waitStart = CpuProfile->ReadMicroseconds();
#endif
    rdpq_detach_wait();
#if HELENGINE_N64_CPU_PROFILE
    const std::uint64_t waitEnd = CpuProfile->ReadMicroseconds();
    if (waitEnd >= waitStart) CpuProfile->RecordNativeStage(Nintendo64CpuProfileSink::RdpqDetachWait, waitEnd - waitStart);
    else CpuProfile->ClockFault();
#endif
    display_show(CurrentFrame);
    CurrentFrame = nullptr;
    return true;
}

void Nintendo64Video::Shutdown() {
    if (CurrentFrame != nullptr) {
        rdpq_detach_wait();
        CurrentFrame = nullptr;
    }

    if (RdpqInitialized) {
        rdpq_close();
        RdpqInitialized = false;
    }
    if (DisplayInitialized) {
        display_close();
        DisplayInitialized = false;
    }
    DepthSurface = nullptr;
}

} // namespace helengine::n64
