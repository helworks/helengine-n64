#pragma once

#include "Nintendo64Renderer.hpp"
#include "Nintendo64Video.hpp"

namespace helengine::n64 {

class Nintendo64BootHost {
public:
    int Run();

private:
    Nintendo64Video Video;
    Nintendo64Renderer Renderer;
};

} // namespace helengine::n64
