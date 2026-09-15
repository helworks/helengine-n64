#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "IInputBackend.hpp"

namespace helengine::n64 {

class Nintendo64InputBackend final : public ::IInputBackend {
public:
    ::InputFrameState CaptureFrame() override;
};

} // namespace helengine::n64

#endif
