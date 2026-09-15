#if defined(HELENGINE_N64_AUTHORED) && HELENGINE_N64_HAS_GENERATED_CORE
#include "platform/n64/Nintendo64RuntimeHost.hpp"
#else
#include "platform/n64/Nintendo64BootHost.hpp"
#endif

int main() {
#if defined(HELENGINE_N64_AUTHORED) && HELENGINE_N64_HAS_GENERATED_CORE
    helengine::n64::Nintendo64RuntimeHost host;
#else
    helengine::n64::Nintendo64BootHost host;
#endif
    return host.Run();
}
