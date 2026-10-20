#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "RenderManager3D.hpp"
#include "IRenderVisitor3D.hpp"
#include "ICamera.hpp"

#include "platform/n64/Nintendo64FrameLights.hpp"

namespace helengine::n64 {

class Nintendo64Renderer;
class Nintendo64Video;
class Nintendo64RenderManager2D;

class Nintendo64RenderManager3D final : public ::RenderManager3D, public ::IRenderVisitor3D {
public:
    explicit Nintendo64RenderManager3D(Nintendo64Renderer* renderer, Nintendo64Video* video = nullptr,
                                      Nintendo64RenderManager2D* render2D = nullptr);

    ::RuntimeMaterial* BuildMaterialFromCooked(::PlatformMaterialAsset* materialAsset) override;
    ::RuntimeMaterial* BuildMaterialFromCooked(HeCppString cookedAssetPath, ::IContentStreamSource* contentStreamSource) override;
    ::RuntimeMaterial* BuildMaterialFromRawAsset(::ContentManager* assetContentManager, HeCppString materialAssetPath) override;
    ::RuntimeModel* BuildModelFromCooked(HeCppString cookedAssetPath, ::IContentStreamSource* contentStreamSource) override;
    ::RuntimeModel* BuildModelFromRaw(::ModelAsset* data) override;
    void Dispose() override;
    void Draw() override;
    void ReleaseMaterial(::RuntimeMaterial* material) override;
    void ReleaseModel(::RuntimeModel* model) override;
    void Visit(::IDrawable3D* drawable) override;

private:
    Nintendo64Renderer* Renderer;
    /// Borrowed host-owned presentation and 2D managers, valid for this manager's lifetime.
    Nintendo64Video* Video;
    Nintendo64RenderManager2D* Render2D;
    int32_t DrawCallCount = 0;
    bool HasLoggedAuthoredDraw = false;
    /// Ambient and directional lights extracted once per Draw and shared by every lit drawable in that frame.
    Nintendo64FrameLights FrameLights;

    static uint32_t PackColor(const ::PlatformMaterialAsset* material);
    ::RuntimeMaterial* BuildMaterialAsset(::PlatformMaterialAsset* materialAsset, ::IContentStreamSource* source);
    bool ConfigureCamera(::ICamera* camera);
};

} // namespace helengine::n64

#endif
