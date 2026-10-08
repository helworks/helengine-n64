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
#include "Entity.hpp"
#include "FontAsset.hpp"
#include "TextComponent.hpp"
#include "TextLayoutAlignmentUtils.hpp"
#include "MenuComponent.hpp"
#include "runtime/native_list.hpp"
#include "system/math.hpp"

#include "platform/n64/Nintendo64ContentStreamSource.hpp"
#include "platform/n64/Nintendo64FrameLights.hpp"
#include "platform/n64/Nintendo64InputBackend.hpp"
#include "platform/n64/Nintendo64RenderManager2D.hpp"
#include "platform/n64/Nintendo64RenderManager3D.hpp"
#include "platform/n64/Nintendo64SceneBootstrap.hpp"
#if HELENGINE_N64_CPU_PROFILE
#include "platform/n64/Nintendo64CpuProfileSink.hpp"
#endif

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
#if HELENGINE_N64_CPU_PROFILE
    CpuProfile = new Nintendo64CpuProfileSink();
    InitializationOptions->set_CpuProfileSink(CpuProfile);
    Video.SetCpuProfileSink(CpuProfile);
#endif
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
    if (cameraCount != 1 || drawableCount > 1) {
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
#if HELENGINE_N64_CPU_PROFILE
    delete CpuProfile;
    CpuProfile = nullptr;
#endif
}

int Nintendo64RuntimeHost::Run() {
    if (!Initialize()) {
        Shutdown();
        return 1;
    }
    bool hasLoggedCompletedFrame = false;
    // Report the live button's imported metrics and applied viewport scale once, without per-frame allocation.
    bool hasLoggedButtonFontMetrics = false;
    // Each heading is measured once after the menu's update-time fit, using the shared renderer's metrics.
    uint8_t loggedTitleMetrics = 0;
    // Initial viewport scaling can complete after the menu update; sample headings after three completed frames.
    uint8_t completedDiagnosticFrames = 0;
    std::string lastMenuPanel;
    std::string lastMenuSelection;
#if HELENGINE_N64_CPU_PROFILE
    std::uint64_t frameSequence = 0u;
#endif
    while (true) {
        if (!Video.BeginFrame()) {
            Shutdown();
            return 1;
        }
        Video.Clear();
#if HELENGINE_N64_CPU_PROFILE
        ++frameSequence;
        if (CpuProfile->HasPendingReport()) CpuProfile->EmitPendingReport();
        CpuProfile->BeginFrame(frameSequence);
        const std::uint64_t updateStart = CpuProfile->ReadMicroseconds();
#endif
        EngineCore->Update(1.0 / 60.0);
        // State changes are useful on hardware's ISViewer/USB log too: emit once
        // per menu transition, rather than printing continuously in the frame loop.
        auto* entities = EngineCore->get_ObjectManager()->get_Entities();
        for (int32_t entityIndex = 0; entities != nullptr && entityIndex < entities->get_Count(); ++entityIndex) {
            auto* entity = entities->get_Item(entityIndex);
            auto* components = entity == nullptr ? nullptr : entity->get_Components();
            for (int32_t componentIndex = 0; components != nullptr && componentIndex < components->get_Count(); ++componentIndex) {
                if (completedDiagnosticFrames >= 3 && loggedTitleMetrics != 3) {
                    auto* title = dynamic_cast<::TextComponent*>(components->get_Item(componentIndex));
                    const uint8_t titleBit = title == nullptr ? 0 : title->get_Text() == "HELENGINE" ? 1 : title->get_Text() == "DEMO DISC" ? 2 : 0;
                    if (titleBit != 0 && (loggedTitleMetrics & titleBit) == 0 && title->get_Font() != nullptr) {
                        const auto size = title->get_Size();
                        const auto position = entity->get_Position();
                        const auto shadow = title->get_ShadowOffset();
                        const double width = ::TextLayoutAlignmentUtils::MeasureVisibleLineWidth(title->get_Text(), title->get_Font(), title->get_FontScale(), title->get_Font()->get_AtlasWidth());
                        const double rightEffect = std::max(static_cast<double>(title->get_OutlineScale()), static_cast<double>(shadow.X));
                        const double lineOffset = ::TextLayoutAlignmentUtils::ResolveHorizontalOffset(title->get_Alignment(), size.X, width);
                        const double snappedBaseX = Math::Round(position.X);
                        const double paintedRight = snappedBaseX + lineOffset + width + rightEffect;
                        auto* titleParent = entity->get_Parent();
                        const double parentRightAnchor = titleParent == nullptr ? static_cast<double>(position.X + size.X) : static_cast<double>(titleParent->get_Position().X);
                        debugf("n64:title-font text=%s scale=%.8f size=%ldx%ld position=%.8f/%.8f width=%.8f outline=%.8f right-effect=%.8f right-with-effect=%.8f box-right=%.8f\n",
                               title->get_Text().c_str(), static_cast<double>(title->get_FontScale()), static_cast<long>(size.X), static_cast<long>(size.Y),
                               static_cast<double>(position.X), static_cast<double>(position.Y), width, static_cast<double>(title->get_OutlineScale()), rightEffect,
                               paintedRight, static_cast<double>(position.X + size.X));
                        debugf("n64:title-right text=%s alignment=%ld snapped-base=%.8f line-offset=%.8f painted-right=%.8f parent-anchor=%.8f margin=%.8f\n",
                               title->get_Text().c_str(), static_cast<long>(title->get_Alignment()), snappedBaseX, lineOffset, paintedRight, parentRightAnchor, parentRightAnchor - paintedRight);
                        loggedTitleMetrics |= titleBit;
                    }
                }
                if (!hasLoggedButtonFontMetrics) {
                    auto* text = dynamic_cast<::TextComponent*>(components->get_Item(componentIndex));
                    if (text != nullptr && text->get_Text() == "Rendering" && text->get_Font() != nullptr) {
                        auto* font = text->get_Font();
                        auto* characters = font->get_Characters();
                        if (characters != nullptr && characters->ContainsKey('R')) {
                            const auto glyph = characters->get_Item('R');
                            const float glyphHeight = glyph.SourceRect.W * font->get_AtlasHeight();
                            float renderingAdvance = 0.0f;
                            for (const char character : text->get_Text()) {
                                if (characters->ContainsKey(character)) renderingAdvance += characters->get_Item(character).AdvanceWidth;
                            }
                            debugf("n64:button-font text=Rendering scale=%.8f line=%.8f atlas=%ldx%ld glyph-height=%.8f bound-height=%.8f advance=%.8f bound-width=%.8f\n",
                                   static_cast<double>(text->get_FontScale()), static_cast<double>(font->get_LineHeight()),
                                   static_cast<long>(font->get_AtlasWidth()), static_cast<long>(font->get_AtlasHeight()),
                                   static_cast<double>(glyphHeight), static_cast<double>(glyphHeight * text->get_FontScale()),
                                   static_cast<double>(renderingAdvance), static_cast<double>(renderingAdvance * text->get_FontScale()));
                            hasLoggedButtonFontMetrics = true;
                        }
                    }
                }
                auto* menu = dynamic_cast<::MenuComponent*>(components->get_Item(componentIndex));
                if (menu == nullptr || !menu->get_IsInitialized()) continue;
                const auto& panel = menu->get_ActivePanelId();
                const auto& selection = menu->get_SelectedItemId();
                if (panel != lastMenuPanel || selection != lastMenuSelection) {
                    debugf("n64:menu-state panel=%s selected=%s\n", panel.c_str(), selection.c_str());
                    lastMenuPanel = panel;
                    lastMenuSelection = selection;
                }
            }
        }
#if HELENGINE_N64_CPU_PROFILE
        const std::uint64_t updateEnd = CpuProfile->ReadMicroseconds();
        if (updateEnd >= updateStart) CpuProfile->RecordNativeStage(Nintendo64CpuProfileSink::HostUpdate, updateEnd - updateStart);
        else CpuProfile->ClockFault();
#endif
        // Core::Draw only runs the 3D render manager; RenderManager2D::Draw is a hook each host drives itself.
        // The screen-space 2D pass therefore runs here, after the Tiny3D pass has queued its triangles and
        // while the frame buffer is still attached, so the sprite blits land over the rendered 3D scene.
        EngineCore->Draw();
        ::RenderManager2D* renderManager2D = EngineCore->get_RenderManager2D();
        if (renderManager2D != nullptr) {
#if HELENGINE_N64_CPU_PROFILE
            const std::uint64_t draw2dStart = CpuProfile->ReadMicroseconds();
#endif
            renderManager2D->Draw();
#if HELENGINE_N64_CPU_PROFILE
            const std::uint64_t draw2dEnd = CpuProfile->ReadMicroseconds();
            if (draw2dEnd >= draw2dStart) CpuProfile->RecordNativeStage(Nintendo64CpuProfileSink::RenderManager2D, draw2dEnd - draw2dStart);
            else CpuProfile->ClockFault();
#endif
        }
        if (!Video.EndFrame()) {
            Shutdown();
            return 1;
        }
        // Scene operations are intentionally not committed during Draw: resources may still be in use by
        // the RDP until EndFrame's detach-and-wait finishes. Commit now, at that completed frame boundary.
        EngineCore->CompleteFrameBoundary();
        if (completedDiagnosticFrames < 3) ++completedDiagnosticFrames;
#if HELENGINE_N64_CPU_PROFILE
        CpuProfile->EndFrame(frameSequence);
#endif
        if (!hasLoggedCompletedFrame) {
            debugf("n64:first-frame-complete\n");
            hasLoggedCompletedFrame = true;
        }
    }
}

} // namespace helengine::n64

#endif
