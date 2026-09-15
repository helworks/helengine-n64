#pragma once

#include "Nintendo64Video.hpp"

namespace helengine::n64 {

class Nintendo64BootHost {
public:
    int Run();

private:
    Nintendo64Video Video;
};

} // namespace helengine::n64
