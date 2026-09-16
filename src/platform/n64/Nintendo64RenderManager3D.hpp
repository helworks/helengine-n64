#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <string>

#include "RenderManager3D.hpp"
#include "IRenderVisitor3D.hpp"
#include "ICamera.hpp"

#include "platform/n64/Nintendo64FrameLights.hpp"

namespace helengine::n64 {

class Nintendo64Renderer;

class Nintendo64RenderManager3D final : public ::RenderManager3D, public ::IRenderVisitor3D {
public:
    explicit Nintendo64RenderManager3D(Nintendo64Renderer* renderer);

    ::RuntimeMaterial* BuildMaterialFromCooked(::PlatformMaterialAsset* materialAsset) override;
    ::RuntimeMaterial* BuildMaterialFromCooked(std::string cookedAssetPath, ::IContentStreamSource* contentStreamSource) override;
    ::RuntimeMaterial* BuildMaterialFromRawAsset(::ContentManager* assetContentManager, std::string materialAssetPath) override;
    ::RuntimeModel* BuildModelFromCooked(std::string cookedAssetPath, ::IContentStreamSource* contentStreamSource) override;
    ::RuntimeModel* BuildModelFromRaw(::ModelAsset* data) override;
    void Dispose() override;
    void Draw() override;
    void ReleaseMaterial(::RuntimeMaterial* material) override;
    void ReleaseModel(::RuntimeModel* model) override;
    void Visit(::IDrawable3D* drawable) override;

private:
    Nintendo64Renderer* Renderer;
    int32_t DrawCallCount = 0;
    bool HasLoggedAuthoredDraw = false;
    /// Ambient and directional lights extracted once per Draw and shared by every lit drawable in that frame.
    Nintendo64FrameLights FrameLights;

    static uint32_t PackColor(const ::PlatformMaterialAsset* material);
    ::RuntimeMaterial* BuildMaterialAsset(::PlatformMaterialAsset* materialAsset);
    bool ConfigureCamera(::ICamera* camera);
};

} // namespace helengine::n64

#endif
