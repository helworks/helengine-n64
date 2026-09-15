#include "platform/n64/Nintendo64RenderManager3D.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cmath>

#include <libdragon.h>
#include <t3d/t3dmath.h>

#include "AssetSerializer.hpp"
#include "ContentManager.hpp"
#include "Core.hpp"
#include "Entity.hpp"
#include "ICamera.hpp"
#include "IDrawable3D.hpp"
#include "IContentStreamSource.hpp"
#include "IRenderQueue3D.hpp"
#include "MeshComponent.hpp"
#include "ModelAsset.hpp"
#include "ObjectManager.hpp"
#include "PlatformMaterialAsset.hpp"
#include "RuntimeMaterial.hpp"
#include "RuntimeMaterialLightingModel.hpp"
#include "RuntimeModel.hpp"
#include "runtime/array.hpp"
#include "runtime/native_cast.hpp"
#include "runtime/native_exceptions.hpp"
#include "system/io/stream.hpp"

#include "platform/n64/Nintendo64Renderer.hpp"
#include "platform/n64/Nintendo64RuntimeMaterial.hpp"
#include "platform/n64/Nintendo64RuntimeModel.hpp"

namespace helengine::n64 {
namespace {

constexpr const char kN64RendererFamilyId[] = "n64-standard";

::Asset* Deserialize(::IContentStreamSource* source, const std::string& path) {
    if (source == nullptr) {
        throw new ArgumentNullException("contentStreamSource");
    }
    ::Stream* stream = source->OpenRead(path);
    if (stream == nullptr) {
        throw new InvalidOperationException("N64 content source returned a null stream.");
    }
    return ::AssetSerializer::Deserialize(stream);
}

} // namespace

Nintendo64RenderManager3D::Nintendo64RenderManager3D(Nintendo64Renderer* renderer)
    : Renderer(renderer) {
    if (Renderer == nullptr) {
        throw new ArgumentNullException("renderer");
    }
}

uint32_t Nintendo64RenderManager3D::PackColor(const ::PlatformMaterialAsset* material) {
    if (material == nullptr) {
        return 0xFFFFFFFFu;
    }
    return (static_cast<uint32_t>(material->BaseColorR) << 24) |
           (static_cast<uint32_t>(material->BaseColorG) << 16) |
           (static_cast<uint32_t>(material->BaseColorB) << 8) |
           static_cast<uint32_t>(material->BaseColorA);
}

::RuntimeMaterial* Nintendo64RenderManager3D::BuildMaterialAsset(::PlatformMaterialAsset* materialAsset) {
    if (materialAsset == nullptr) {
        throw new ArgumentNullException("materialAsset");
    }
    if (!materialAsset->TextureRelativePath.empty() || materialAsset->RendererFamilyId != kN64RendererFamilyId) {
        throw new InvalidOperationException("N64 authored scenes require opaque n64-standard materials without textures.");
    }
    debugf("n64:material-loaded id=%s color=%08lX lit=%d\n", materialAsset->get_Id().c_str(), static_cast<unsigned long>(PackColor(materialAsset)), materialAsset->Lit ? 1 : 0);
    return new Nintendo64RuntimeMaterial(PackColor(materialAsset), materialAsset->DoubleSided, materialAsset->Lit);
}

::RuntimeMaterial* Nintendo64RenderManager3D::BuildMaterialFromCooked(::PlatformMaterialAsset* materialAsset) {
    return BuildMaterialAsset(materialAsset);
}

::RuntimeMaterial* Nintendo64RenderManager3D::BuildMaterialFromCooked(
    std::string cookedAssetPath, ::IContentStreamSource* contentStreamSource) {
    ::Asset* rawAsset = Deserialize(contentStreamSource, cookedAssetPath);
    ::PlatformMaterialAsset* material = he_cpp_try_cast<::PlatformMaterialAsset>(rawAsset);
    if (material == nullptr) {
        delete rawAsset;
        throw new InvalidOperationException("N64 cooked material asset has the wrong type.");
    }
    ::RuntimeMaterial* runtimeMaterial = BuildMaterialAsset(material);
    delete material;
    return runtimeMaterial;
}

::RuntimeMaterial* Nintendo64RenderManager3D::BuildMaterialFromRawAsset(
    ::ContentManager*, std::string) {
    throw new NotSupportedException("N64 authored scenes do not load raw material assets.");
}

::RuntimeModel* Nintendo64RenderManager3D::BuildModelFromCooked(
    std::string cookedAssetPath, ::IContentStreamSource* contentStreamSource) {
    ::Asset* rawAsset = Deserialize(contentStreamSource, cookedAssetPath);
    ::ModelAsset* modelAsset = he_cpp_try_cast<::ModelAsset>(rawAsset);
    if (modelAsset == nullptr) {
        delete rawAsset;
        throw new InvalidOperationException("N64 cooked model asset has the wrong type.");
    }
    Nintendo64RuntimeModel* runtimeModel = new Nintendo64RuntimeModel();
    if (!runtimeModel->Initialize(modelAsset)) {
        modelAsset->Dispose();
        delete modelAsset;
        delete runtimeModel;
        throw new InvalidOperationException("N64 cooked model exceeds the authored Tiny3D mesh limits.");
    }
    debugf("n64:model-loaded id=%s vertices=%ld indices=%ld\n", modelAsset->get_Id().c_str(), static_cast<long>(modelAsset->Positions->get_Length()), static_cast<long>(modelAsset->Indices16->get_Length()));
    return runtimeModel;
}

::RuntimeModel* Nintendo64RenderManager3D::BuildModelFromRaw(::ModelAsset*) {
    throw new NotSupportedException("N64 authored scenes do not build raw model assets.");
}

void Nintendo64RenderManager3D::Dispose() {
    DrawCallCount = 0;
    HasLoggedAuthoredDraw = false;
}

bool Nintendo64RenderManager3D::ConfigureCamera(::ICamera* camera) {
    if (camera == nullptr || camera->get_Parent() == nullptr || camera->get_RenderQueue3D() == nullptr) {
        return false;
    }
    ::Entity* parent = camera->get_Parent();
    const ::float3 position = parent->get_Position();
    const ::float4 orientation = parent->get_Orientation();
    const ::float3 forward = ::float4::RotateVector(::float3(0.0f, 0.0f, -1.0f), orientation);
    const ::float3 up = ::float4::RotateVector(::float3(0.0f, 1.0f, 0.0f), orientation);
    const ::float3 target = position + forward;
    T3DVec3 eye = {{position.X, position.Y, position.Z}};
    T3DVec3 lookTarget = {{target.X, target.Y, target.Z}};
    T3DVec3 lookUp = {{up.X, up.Y, up.Z}};
    T3DMat4 viewMatrix;
    t3d_mat4_look_at(&viewMatrix, &eye, &lookTarget, &lookUp);

    const ::float4 viewport = camera->get_Viewport();
    const bool normalizedViewport = viewport.Z > 0.0f && viewport.Z <= 1.0f && viewport.W > 0.0f && viewport.W <= 1.0f;
    const float width = normalizedViewport ? 320.0f : (viewport.Z > 0.0f ? viewport.Z : 320.0f);
    const float height = normalizedViewport ? 240.0f : (viewport.W > 0.0f ? viewport.W : 240.0f);
    const float aspect = width / height;
    const float nearPlane = camera->get_NearPlaneDistance() > 0.0f ? camera->get_NearPlaneDistance() : 0.1f;
    const float farPlane = camera->get_FarPlaneDistance() > nearPlane ? camera->get_FarPlaneDistance() : 1000.0f;
    Renderer->SetAuthoredCamera(viewMatrix, T3D_DEG_TO_RAD(60.0f), aspect, nearPlane, farPlane);
    return true;
}

void Nintendo64RenderManager3D::Draw() {
    DrawCallCount = 0;
    if (OwnerCore == nullptr || OwnerCore->get_ObjectManager() == nullptr || Renderer == nullptr) {
        return;
    }
    ::List<::ICamera*>* cameras = OwnerCore->get_ObjectManager()->get_Cameras();
    if (cameras == nullptr || cameras->empty()) {
        return;
    }
    ::ICamera* camera = (*cameras)[0];
    if (!ConfigureCamera(camera)) {
        return;
    }
    Renderer->BeginScene();
    camera->get_RenderQueue3D()->VisitOrdered(this);
}

void Nintendo64RenderManager3D::ReleaseMaterial(::RuntimeMaterial* material) {
    if (material == nullptr) {
        return;
    }
    material->Dispose();
    delete material;
}

void Nintendo64RenderManager3D::ReleaseModel(::RuntimeModel* model) {
    if (model == nullptr) {
        return;
    }
    model->Dispose();
    delete model;
}

void Nintendo64RenderManager3D::Visit(::IDrawable3D* drawable) {
    if (drawable == nullptr) {
        return;
    }
    if (drawable->get_Parent() == nullptr) {
        return;
    }
    ::RuntimeModel* baseModel = drawable->get_Model();
    if (baseModel == nullptr) {
        return;
    }
    Nintendo64RuntimeModel* model = he_cpp_try_cast<Nintendo64RuntimeModel>(drawable->get_Model());
    if (model == nullptr) {
        return;
    }
    uint32_t color = 0xFFFFFFFFu;
    bool doubleSided = false;
    ::Array<::RuntimeMaterial*>* materials = drawable->get_Materials();
    if (materials != nullptr && materials->get_Length() > 0 && (*materials)[0] != nullptr) {
        Nintendo64RuntimeMaterial* material = he_cpp_try_cast<Nintendo64RuntimeMaterial>((*materials)[0]);
        if (material != nullptr) {
            color = material->Color();
            doubleSided = material->DoubleSided();
        }
    }
    model->SetColor(color);

    ::Entity* entity = drawable->get_Parent();
    const ::float3 position = entity->get_Position();
    const ::float3 scale = entity->get_Scale();
    const ::float4 orientation = entity->get_Orientation();
    const float authoredScale[3] = {scale.X / 64.0f, scale.Y / 64.0f, scale.Z / 64.0f};
    const float authoredRotation[4] = {orientation.X, orientation.Y, orientation.Z, orientation.W};
    const float authoredPosition[3] = {position.X, position.Y, position.Z};
    T3DMat4 modelMatrix;
    t3d_mat4_from_srt(&modelMatrix, authoredScale, authoredRotation, authoredPosition);
    Renderer->DrawAuthoredMesh(model->Vertices(), model->VertexCount(), model->Indices(), model->IndexCount(), modelMatrix, color, doubleSided);
    DrawCallCount++;
    if (!HasLoggedAuthoredDraw) {
        ::ModelAsset* rawModel = model->get_RawModelAsset();
        const char* modelId = rawModel == nullptr ? "<unknown>" : rawModel->get_Id().c_str();
        debugf("n64:first-authored-draw model=%s material=n64-unlit calls=%ld\n", modelId, static_cast<long>(DrawCallCount));
        HasLoggedAuthoredDraw = true;
    }
}

} // namespace helengine::n64

#endif
