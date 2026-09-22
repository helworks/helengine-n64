#include "platform/n64/Nintendo64RuntimeHost.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <algorithm>
#include <cmath>

#include <libdragon.h>

#include "AmbientLightComponent.hpp"
#include "CameraClearSettings.hpp"
#include "Core.hpp"
#include "CoreInitializationOptions.hpp"
#include "DirectionalLightComponent.hpp"
#include "ICamera.hpp"
#include "PlatformInfo.hpp"
#include "RuntimeSceneCatalog.hpp"
#include "SceneLoadMode.hpp"
#include "SceneManager.hpp"
#include "ObjectManager.hpp"
#include "runtime/native_list.hpp"

#include "platform/n64/Nintendo64ContentStreamSource.hpp"
#include "platform/n64/Nintendo64FrameLights.hpp"
#include "platform/n64/Nintendo64InputBackend.hpp"
#include "platform/n64/Nintendo64RenderManager2D.hpp"
#include "platform/n64/Nintendo64RenderManager3D.hpp"
#include "platform/n64/Nintendo64SceneBootstrap.hpp"

extern "C" const char* he_get_runtime_startup_scene_id();

namespace helengine::n64 {

namespace {

uint8_t ToRgbaByte(float channel) {
    const float clamped = std::clamp(channel, 0.0f, 1.0f);
    return static_cast<uint8_t>(std::lround(clamped * 255.0f));
}

} // namespace

bool Nintendo64RuntimeHost::Initialize() {
    debug_init_isviewer();
    debug_init_usblog();
    debugf("n64:authored-runtime-start\n");
    if (!Video.Initialize()) {
        return false;
    }
    debugf("n64:video-ready\n");
    if (!Renderer.InitializeAuthored()) {
        return false;
    }
    debugf("n64:renderer-ready\n");
    const int dragonFsResult = dfs_init(DFS_DEFAULT_LOCATION);
    if (dragonFsResult != DFS_ESUCCESS) {
        debugf("n64:error dragonfs-initialization-failed code=%ld\n", static_cast<long>(dragonFsResult));
        return false;
    }
    debugf("n64:dragonfs-ready\n");

    InitializationOptions = new ::CoreInitializationOptions();
    InitializationOptions->set_ContentStreamSource(new Nintendo64ContentStreamSource("rom:/"));
    ContentSource = static_cast<Nintendo64ContentStreamSource*>(InitializationOptions->get_ContentStreamSource());
    InitializationOptions->set_UpdateOrderLayers(4);
    InitializationOptions->set_RenderOrderLayers3D(4);
    InitializationOptions->set_UpdateListInitialCapacity(16);
    InitializationOptions->set_RenderList2DInitialCapacity(0);
    InitializationOptions->set_RenderList3DInitialCapacity(64);
    InitializationOptions->set_DefaultUpdateDeltaSeconds(1.0 / 60.0);
    InitializationOptions->set_PhysicsFixedStepSeconds(1.0 / 60.0);
    InitializationOptions->set_PhysicsMaxStepsPerUpdate(1);
    SceneCatalog = BuildNintendo64RuntimeSceneCatalog();
    if (SceneCatalog == nullptr) {
        debugf("n64:error runtime-scene-catalog-missing\n");
        return false;
    }
    InitializationOptions->set_SceneCatalog(SceneCatalog);
    InitializationOptions->set_CommitPendingSceneOperationsDuringDraw(false);

    Render3D = new Nintendo64RenderManager3D(&Renderer);
    Render2D = new Nintendo64RenderManager2D();
    Input = new Nintendo64InputBackend();
    Platform = new ::PlatformInfo("n64", "0.2.0");
    Render3D->AddWindow(0, 320, 240);

    EngineCore = new ::Core(InitializationOptions);
    EngineCore->Initialize(Render3D, Render2D, Input, Platform, InitializationOptions);
    debugf("n64:core-ready\n");
    const char* startupSceneId = he_get_runtime_startup_scene_id();
    if (startupSceneId == nullptr || startupSceneId[0] == '\0') {
        debugf("n64:error runtime-startup-scene-missing\n");
        return false;
    }
    debugf("n64:startup-scene id=%s\n", startupSceneId);
    EngineCore->get_SceneManager()->LoadScene(startupSceneId, ::SceneLoadMode::Single);
    EngineCore->get_SceneManager()->CommitPendingOperationsAtFrameBoundary();
    ::ObjectManager* objectManager = EngineCore->get_ObjectManager();
    const long cameraCount = objectManager == nullptr || objectManager->get_Cameras() == nullptr
        ? 0
        : static_cast<long>(objectManager->get_Cameras()->get_Count());
    const long drawableCount = objectManager == nullptr || objectManager->get_Drawables3D() == nullptr
        ? 0
        : static_cast<long>(objectManager->get_Drawables3D()->get_Count());
    const long drawable2dCount = objectManager == nullptr || objectManager->get_Drawables2D() == nullptr
        ? 0
        : static_cast<long>(objectManager->get_Drawables2D()->get_Count());
    const long directionalLightCount = objectManager == nullptr || objectManager->get_DirectionalLights() == nullptr
        ? 0
        : static_cast<long>(objectManager->get_DirectionalLights()->get_Count());
    const long ambientLightCount = objectManager == nullptr || objectManager->get_AmbientLights() == nullptr
        ? 0
        : static_cast<long>(objectManager->get_AmbientLights()->get_Count());
    debugf("n64:scene-ready cameras=%ld drawables3d=%ld drawables2d=%ld directional=%ld ambient=%ld\n", cameraCount, drawableCount, drawable2dCount, directionalLightCount, ambientLightCount);
    if (cameraCount != 1 || drawableCount != 1) {
        debugf("n64:error authored-scene-cardinality\n");
        return false;
    }
    ::ICamera* camera = (*objectManager->get_Cameras()).get_Item(0);
    if (camera != nullptr) {
        ::CameraClearSettings clearSettings = camera->get_ClearSettings();
        if (clearSettings.get_ClearColorEnabled()) {
            const ::float4 clearColor = clearSettings.get_ClearColor();
            const uint8_t red = ToRgbaByte(clearColor.X);
            const uint8_t green = ToRgbaByte(clearColor.Y);
            const uint8_t blue = ToRgbaByte(clearColor.Z);
            const uint8_t alpha = ToRgbaByte(clearColor.W);
            Video.SetClearColor(red, green, blue, alpha);
            debugf("n64:camera-clear rgba=%ld/%ld/%ld/%ld\n",
                   static_cast<long>(red), static_cast<long>(green),
                   static_cast<long>(blue), static_cast<long>(alpha));
        }
    }
    if (directionalLightCount > kMaxDirectionalLights) {
        debugf("n64:error directional-light-limit count=%ld\n", directionalLightCount);
        return false;
    }
    return true;
}

void Nintendo64RuntimeHost::Shutdown() {
    if (EngineCore != nullptr) {
        EngineCore->Dispose();
        delete EngineCore;
        EngineCore = nullptr;
    }
    delete Render3D;
    Render3D = nullptr;
    delete Render2D;
    Render2D = nullptr;
    delete Input;
    Input = nullptr;
    delete Platform;
    Platform = nullptr;
    if (SceneCatalog != nullptr) {
        SceneCatalog->Dispose();
        delete SceneCatalog;
        SceneCatalog = nullptr;
    }
    delete ContentSource;
    ContentSource = nullptr;
    InitializationOptions = nullptr;
    Renderer.Shutdown();
    Video.Shutdown();
}

int Nintendo64RuntimeHost::Run() {
    if (!Initialize()) {
        Shutdown();
        return 1;
    }
    bool hasLoggedCompletedFrame = false;
    while (true) {
        if (!Video.BeginFrame()) {
            Shutdown();
            return 1;
        }
        Video.Clear();
        EngineCore->Update(1.0 / 60.0);
        // Core::Draw only runs the 3D render manager; RenderManager2D::Draw is a hook each host drives itself.
        // The screen-space 2D pass therefore runs here, after the Tiny3D pass has queued its triangles and
        // while the frame buffer is still attached, so the sprite blits land over the rendered 3D scene.
        EngineCore->Draw();
        ::RenderManager2D* renderManager2D = EngineCore->get_RenderManager2D();
        if (renderManager2D != nullptr) {
            renderManager2D->Draw();
        }
        if (!Video.EndFrame()) {
            Shutdown();
            return 1;
        }
        if (!hasLoggedCompletedFrame) {
            debugf("n64:first-frame-complete\n");
            hasLoggedCompletedFrame = true;
        }
    }
}

} // namespace helengine::n64

#endif
