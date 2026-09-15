#pragma once

#include "FrameClock.hpp"
#include "Nintendo64Renderer.hpp"
#include "Nintendo64Video.hpp"

namespace helengine::n64 {

class Nintendo64BootHost {
public:
    int Run();

private:
    Nintendo64Video Video;
    Nintendo64Renderer Renderer;
    FrameClock Clock;
};

} // namespace helengine::n64

