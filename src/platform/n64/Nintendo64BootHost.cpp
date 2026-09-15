#include "Nintendo64BootHost.hpp"

namespace helengine::n64 {

int Nintendo64BootHost::Run() {
    debug_init_isviewer();
    debug_init_usblog();
    debugf("n64:startup\n");

    if (!Video.Initialize()) {
        debugf("n64:error video-initialization-failed\n");
        return 1;
    }
    debugf("n64:video-ready\n");

    if (!Renderer.Initialize()) {
        debugf("n64:error renderer-initialization-failed\n");
        Video.Shutdown();
        return 2;
    }
    debugf("n64:renderer-ready\n");

    Renderer.Prepare(0.0f, 0.0f);
    bool firstFrame = true;
    for (;;) {
        if (!Video.BeginFrame()) {
            Renderer.Shutdown();
            Video.Shutdown();
            return 3;
        }
        Renderer.BeginScene();
        Video.Clear();
        Renderer.DrawCube();
        if (!Video.EndFrame()) {
            Renderer.Shutdown();
            Video.Shutdown();
            return 4;
        }
        if (firstFrame) {
            debugf("n64:first-frame-complete\n");
            firstFrame = false;
        }
    }
}

} // namespace helengine::n64
