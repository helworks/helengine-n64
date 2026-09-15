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

    bool firstFrame = true;
    for (;;) {
        if (!Video.BeginFrame()) {
            Video.Shutdown();
            return 2;
        }
        Video.Clear();
        if (!Video.EndFrame()) {
            Video.Shutdown();
            return 3;
        }
        if (firstFrame) {
            debugf("n64:first-frame-complete\n");
            firstFrame = false;
        }
    }
}

} // namespace helengine::n64
