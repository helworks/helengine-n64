#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#include "platform/n64/Nintendo64SceneStartup.hpp"
#include "platform/n64/Nintendo64CameraPass.hpp"
#include "platform/n64/Nintendo64Video.hpp"

static std::vector<std::string> events;
static surface_t colorSurface{}, depthSurface{};
static int attachCount = 0, displayCount = 0, waitCount = 0;
void display_init(int, int, int buffers, int, int) { assert(buffers == 2); }
void display_close() {}
surface_t* display_get() { return &colorSurface; }
surface_t* display_get_zbuf() { return &depthSurface; }
void display_show(surface_t* surface) { assert(surface == &colorSurface); ++displayCount; events.emplace_back("present"); }
void rdpq_init() {}
void rdpq_close() {}
void rdpq_attach(surface_t* color, surface_t* depth) { assert(color == &colorSurface && depth == &depthSurface); ++attachCount; }
void rdpq_detach_wait() { ++waitCount; events.emplace_back("fence"); }
void rdpq_clear(color_t color) { assert(color.r == 17 && color.g == 23 && color.b == 31); events.emplace_back("clear-color"); }
void rdpq_clear_z(uint16_t depth) { assert(depth == ZBUF_MAX); events.emplace_back("clear-depth"); }
void debugf(const char*, ...) {}

struct Manager {
    std::vector<std::string> Calls;
    void LoadScene(const char* id, SceneLoadMode mode) { Calls.emplace_back(std::string(mode == SceneLoadMode::Single ? "single:" : "additive:") + id); }
    void CommitPendingOperationsAtFrameBoundary() { Calls.emplace_back("commit"); }
    void RequestSceneTransition(const char* id) { Calls.emplace_back(std::string("request:") + id); }
};

struct Camera { const char* Name; bool ClearColor; bool ClearDepth; };
struct CameraList {
    std::vector<Camera*> Items;
    int get_Count() { return static_cast<int>(Items.size()); }
    Camera* get_Item(int index) { return Items[static_cast<unsigned>(index)]; }
};

struct ClearSettings {
    bool ColorEnabled;
    bool DepthEnabled;
    bool StencilEnabled;
    bool get_ClearColorEnabled() { return ColorEnabled; }
    bool get_ClearDepthEnabled() { return DepthEnabled; }
    bool get_ClearStencilEnabled() { return StencilEnabled; }
    struct Color { float X, Y, Z, W; };
    Color get_ClearColor() { return {17, 23, 31, 255}; }
    float get_ClearDepth() { return 1.0f; }
};

int main() {
    using namespace helengine::n64;
    for (const char* loading : {static_cast<const char*>(nullptr), "", "Menu"}) {
        Manager manager;
        assert(!StartAuthoredScenes(manager, "Menu", loading));
        assert((manager.Calls == std::vector<std::string>{"single:Menu", "commit"}));
    }
    // Equality is by string value, rather than relying on shared literal addresses.
    std::string same = "Loading";
    Manager direct;
    assert(!StartAuthoredScenes(direct, "Loading", same.c_str()));
    assert((direct.Calls == std::vector<std::string>{"single:Loading", "commit"}));
    for (const char* loading : {"Loading", "MenuExtended", "Men"}) {
        Manager manager;
        assert(StartAuthoredScenes(manager, "Menu", loading));
        assert((manager.Calls == std::vector<std::string>{std::string("additive:") + loading, "commit", "request:Menu"}));
    }

    Nintendo64Video video;
    assert(video.Initialize());
    video.SetClearColor(17, 23, 31);
    const auto channel = [](float value) { return static_cast<uint8_t>(value); };
    // Stencil-enabled authored cameras are valid on a framebuffer without a stencil plane.
    // A stencil-only clear must neither erase color/depth nor prevent the following draw.
    ApplyCameraAttachmentClears(video, ClearSettings{false, false, true}, channel);
    assert(events.empty());
    ApplyCameraAttachmentClears(video, ClearSettings{true, true, true}, channel);
    assert((events == std::vector<std::string>{"clear-color", "clear-depth"}));
    events.clear();
    Camera main{"main", true, true}, loading{"loading", false, false}, cube{"cube", true, true};
    CameraList cameras{{&main, nullptr, &loading}};
    assert(video.BeginFrame());
    RenderOrderedCameraPasses(cameras,
        [&](Camera* camera) { video.ClearAttachments(camera->ClearColor, camera->ClearDepth); },
        [](Camera* camera) { events.emplace_back(std::string(camera->Name) + ":3d"); },
        [](Camera* camera) { events.emplace_back(std::string(camera->Name) + ":2d"); });
    assert(video.EndFrame());
    assert((events == std::vector<std::string>{"clear-color", "clear-depth", "main:3d", "main:2d", "loading:3d", "loading:2d", "fence", "present"}));
    assert(attachCount == 1 && waitCount == 1 && displayCount == 1);
    // Replace the active scene between completed frames: no retained main-camera reference is used.
    events.clear();
    cameras.Items = {&cube, &loading};
    assert(video.BeginFrame());
    RenderOrderedCameraPasses(cameras,
        [&](Camera* camera) { video.ClearAttachments(camera->ClearColor, camera->ClearDepth); },
        [](Camera* camera) { events.emplace_back(std::string(camera->Name) + ":3d"); },
        [](Camera* camera) { events.emplace_back(std::string(camera->Name) + ":2d"); });
    assert(video.EndFrame());
    assert((events == std::vector<std::string>{"clear-color", "clear-depth", "cube:3d", "cube:2d", "loading:3d", "loading:2d", "fence", "present"}));
    assert(attachCount == 2 && waitCount == 2 && displayCount == 2);
    video.Shutdown();
    std::puts("N64 loading startup, ordered camera passes and production video fence/clear tests passed.");
}
