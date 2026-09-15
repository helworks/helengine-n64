#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "Nintendo64Renderer.hpp"
#include "Nintendo64Video.hpp"

class Core;
class CoreInitializationOptions;
class PlatformInfo;
class RuntimeSceneCatalog;
class IContentStreamSource;
class RenderManager3D;
class RenderManager2D;
class IInputBackend;

namespace helengine::n64 {

class Nintendo64ContentStreamSource;
class Nintendo64RenderManager3D;
class Nintendo64RenderManager2D;
class Nintendo64InputBackend;

/// Hosts generated Core and keeps the existing boot host available for diagnostic mode.
class Nintendo64RuntimeHost {
public:
    int Run();

private:
    Nintendo64Video Video;
    Nintendo64Renderer Renderer;
    ::Core* EngineCore = nullptr;
    ::CoreInitializationOptions* InitializationOptions = nullptr;
    ::PlatformInfo* Platform = nullptr;
    ::RuntimeSceneCatalog* SceneCatalog = nullptr;
    Nintendo64ContentStreamSource* ContentSource = nullptr;
    Nintendo64RenderManager3D* Render3D = nullptr;
    Nintendo64RenderManager2D* Render2D = nullptr;
    Nintendo64InputBackend* Input = nullptr;

    bool Initialize();
    void Shutdown();
};

} // namespace helengine::n64

#endif
