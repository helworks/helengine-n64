#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "IInputBackend.hpp"
#include "InputGamepadState.hpp"
#include "runtime/array.hpp"

namespace helengine::n64 {

class Nintendo64InputBackend final : public ::IInputBackend {
public:
    Nintendo64InputBackend();
    ~Nintendo64InputBackend();
    ::InputFrameState CaptureFrame() override;
private:
    ::Array<::InputGamepadState> FrameA{4};
    ::Array<::InputGamepadState> FrameB{4};
    unsigned CapturedFrames = 0;
};

} // namespace helengine::n64

#endif
