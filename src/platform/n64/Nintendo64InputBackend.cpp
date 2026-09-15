#include "platform/n64/Nintendo64InputBackend.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "InputFrameState.hpp"

namespace helengine::n64 {

::InputFrameState Nintendo64InputBackend::CaptureFrame() {
    ::InputFrameState frame;
    frame.GamepadCount = 0;
    return frame;
}

} // namespace helengine::n64

#endif
