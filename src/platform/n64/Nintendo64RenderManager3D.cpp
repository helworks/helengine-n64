#include "platform/n64/Nintendo64RenderManager3D.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cmath>
#include <algorithm>
#include <memory>

#include <libdragon.h>
#include <t3d/t3dmath.h>

#include "AssetSerializer.hpp"
#include "ContentManager.hpp"
#include "Core.hpp"
#include "CoreInitializationOptions.hpp"
#include "CameraClearSettings.hpp"
#include "SceneManager.hpp"
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

#include "platform/n64/Nintendo64LightExtractor.hpp"
#include "platform/n64/Nintendo64CameraPass.hpp"
#include "platform/n64/Nintendo64RenderManager2D.hpp"
#include "platform/n64/Nintendo64Video.hpp"
#include "platform/n64/Nintendo64Renderer.hpp"
#include "platform/n64/Nintendo64RuntimeMaterial.hpp"
#include "platform/n64/Nintendo64RuntimeModel.hpp"
#include "platform/n64/Nintendo64RuntimeTexture.hpp"

namespace helengine::n64 {
namespace {

constexpr const char kN64RendererFamilyId[] = "n64-standard";

::Asset* Deserialize(::IContentStreamSource* source, const HeCppString& path) {
    if (source == nullptr) {
        throw new ArgumentNullException("contentStreamSource");
    }
    ::Stream* stream = source->OpenRead(path);
    if (stream == nullptr) {
        throw new InvalidOperationException("N64 content source returned a null stream.");
    }
    const auto releaseStream = [](::Stream* value) { value->Dispose(); delete value; };
    const std::unique_ptr<::Stream, decltype(releaseStream)> ownedStream(stream, releaseStream);
    return ::AssetSerializer::Deserialize(ownedStream.get());
}

} // namespace

Nintendo64RenderManager3D::Nintendo64RenderManager3D(Nintendo64Renderer* renderer, Nintendo64Video* video,
                                                 Nintendo64RenderManager2D* render2D)
    : Renderer(renderer), Video(video), Render2D(render2D) {
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

::RuntimeMaterial* Nintendo64RenderManager3D::BuildMaterialAsset(::PlatformMaterialAsset* materialAsset, ::IContentStreamSource* source) {
    if (materialAsset == nullptr) {
        throw new ArgumentNullException("materialAsset");
    }
    if (materialAsset->RendererFamilyId != kN64RendererFamilyId) {
        throw new InvalidOperationException("N64 authored scenes require opaque n64-standard materials.");
    }
    auto* material = new Nintendo64RuntimeMaterial(PackColor(materialAsset), materialAsset->DoubleSided, materialAsset->Lit);
    try {
        if (!materialAsset->TextureRelativePath.empty()) {
            if (Render2D == nullptr || source == nullptr) {
                throw new InvalidOperationException("N64 textured materials require a texture manager and cooked content source.");
            }
            ::RuntimeTexture* uploaded = Render2D->BuildTextureFromCooked(materialAsset->TextureRelativePath, source);
            auto* texture = he_cpp_try_cast<Nintendo64RuntimeTexture>(uploaded);
            if (texture == nullptr || texture->Surface() == nullptr || !rdpq_tex_can_upload(texture->Surface())) {
                if (uploaded != nullptr) Render2D->ReleaseTexture(uploaded);
                throw new InvalidOperationException("N64 mesh texture must be an initialized native texture fitting TMEM; cook 64x64 CI4 or a smaller direct texture.");
            }
            material->SetTexture(*Render2D, texture);
        }
    } catch (...) {
        delete material;
        throw;
    }
    debugf("n64:material-loaded id=%s color=%08lX lit=%d textured=%d\n", materialAsset->get_Id().c_str(), static_cast<unsigned long>(PackColor(materialAsset)), materialAsset->Lit ? 1 : 0, material->Texture() != nullptr ? 1 : 0);
    return material;
}

::RuntimeMaterial* Nintendo64RenderManager3D::BuildMaterialFromCooked(::PlatformMaterialAsset* materialAsset) {
    return BuildMaterialAsset(materialAsset, OwnerCore == nullptr || OwnerCore->get_InitializationOptions() == nullptr
        ? nullptr : OwnerCore->get_InitializationOptions()->ContentStreamSource);
}

::RuntimeMaterial* Nintendo64RenderManager3D::BuildMaterialFromCooked(
    HeCppString cookedAssetPath, ::IContentStreamSource* contentStreamSource) {
    ::Asset* rawAsset = Deserialize(contentStreamSource, cookedAssetPath);
    ::PlatformMaterialAsset* material = he_cpp_try_cast<::PlatformMaterialAsset>(rawAsset);
    if (material == nullptr) {
        delete rawAsset;
        throw new InvalidOperationException("N64 cooked material asset has the wrong type.");
    }
    // The runtime material copies every field it needs out of the cooked asset, so the asset is dead weight
    // once the build returns. BuildMaterialAsset can reject a foreign family or unusable texture, so the
    // release has to happen on that path too rather than only on the one that returns.  This mirrors
    // BuildTextureFromCooked in the 2D path; unlike TextureAsset, the generated PlatformMaterialAsset
    // declares no Dispose, so deleting it is the whole release and the two paths stay symmetric by shape.
    ::RuntimeMaterial* runtimeMaterial = nullptr;
    try {
        runtimeMaterial = BuildMaterialAsset(material, contentStreamSource);
    } catch (...) {
        delete material;
        throw;
    }
    delete material;
    return runtimeMaterial;
}

::RuntimeMaterial* Nintendo64RenderManager3D::BuildMaterialFromRawAsset(
    ::ContentManager*, HeCppString) {
    throw new NotSupportedException("N64 authored scenes do not load raw material assets.");
}

::RuntimeModel* Nintendo64RenderManager3D::BuildModelFromCooked(
    HeCppString cookedAssetPath, ::IContentStreamSource* contentStreamSource) {
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
    T3DVec3 eye = {{static_cast<float>(position.X), static_cast<float>(position.Y), static_cast<float>(position.Z)}};
    T3DVec3 lookTarget = {{static_cast<float>(target.X), static_cast<float>(target.Y), static_cast<float>(target.Z)}};
    T3DVec3 lookUp = {{static_cast<float>(up.X), static_cast<float>(up.Y), static_cast<float>(up.Z)}};
    T3DMat4 viewMatrix;
    t3d_mat4_look_at(&viewMatrix, &eye, &lookTarget, &lookUp);

    const ::float4 viewport = camera->get_Viewport();
    const bool normalizedViewport = viewport.Z > 0.0f && viewport.Z <= 1.0f && viewport.W > 0.0f && viewport.W <= 1.0f;
    const auto width = normalizedViewport ? 320.0f : (viewport.Z > 0.0f ? viewport.Z : 320.0f);
    const auto height = normalizedViewport ? 240.0f : (viewport.W > 0.0f ? viewport.W : 240.0f);
    const auto aspect = width / height;
    const auto nearPlane = camera->get_NearPlaneDistance() > 0.0f ? camera->get_NearPlaneDistance() : 0.1f;
    const auto farPlane = camera->get_FarPlaneDistance() > nearPlane ? camera->get_FarPlaneDistance() : 1000.0f;
    Renderer->SetAuthoredCamera(viewMatrix, T3D_DEG_TO_RAD(60.0f), static_cast<float>(aspect), static_cast<float>(nearPlane), static_cast<float>(farPlane));
    return true;
}

void Nintendo64RenderManager3D::Draw() {
    DrawCallCount = 0;
    if (OwnerCore == nullptr || OwnerCore->get_ObjectManager() == nullptr || Renderer == nullptr) {
        return;
    }
    ::List<::ICamera*>* cameras = OwnerCore->get_ObjectManager()->get_Cameras();
    if (cameras == nullptr || cameras->empty()) {
        if (!OwnerCore->get_SceneManager()->get_IsSceneTransitionActive()) {
            throw new InvalidOperationException("N64 authored scene requires a camera.");
        }
        if (Video != nullptr) Video->Clear();
        return;
    }
    Nintendo64LightExtractor::Extract(OwnerCore->get_ObjectManager(), FrameLights);
    RenderOrderedCameraPasses(*cameras,
        [this](::ICamera* camera) {
            if (Video == nullptr) return;
            ApplyCameraAttachmentClears(*Video, camera->get_ClearSettings(), [](RealEvaluation value) {
                return static_cast<uint8_t>(static_cast<int32_t>(RealMath::Round(RealMath::Clamp(value, 0.0f, 1.0f) * 255.0f)));
            });
        },
        [this](::ICamera* camera) {
            if (camera->get_RenderQueue3D() == nullptr || camera->get_RenderQueue3D()->get_Count() == 0) return;
            // Tiny3D's unbuffered viewport and retained model matrix are DMA inputs. The previous
            // camera must finish reading them before the next pass overwrites either allocation.
            rspq_wait();
            if (!ConfigureCamera(camera)) throw new InvalidOperationException("N64 camera has no parent or render queue.");
            Renderer->BeginScene();
            camera->get_RenderQueue3D()->VisitOrdered(this);
        },
        [this](::ICamera* camera) {
            if (Render2D != nullptr) Render2D->DrawCamera(camera);
        });
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
    bool lit = false;
    Nintendo64RuntimeTexture* texture = nullptr;
    ::Array<::RuntimeMaterial*>* materials = drawable->get_Materials();
    if (materials != nullptr && materials->get_Length() > 0 && (*materials)[0] != nullptr) {
        Nintendo64RuntimeMaterial* material = he_cpp_try_cast<Nintendo64RuntimeMaterial>((*materials)[0]);
        if (material != nullptr) {
            color = material->Color();
            doubleSided = material->DoubleSided();
            lit = material->Lit();
            texture = material->Texture();
        }
    }
    if (lit && !model->HasNormals()) {
        throw new InvalidOperationException("N64 lit materials require cooked vertex normals.");
    }
    // Cached models share mutable vertex colors, and the renderer reuses one
    // retained model matrix. Tiny3D consumes both through deferred RSP DMA.
    // Finish the previous submission before changing either input.
    rspq_wait();
    model->SetColor(color);
    if (texture != nullptr) {
        if (texture->Surface() == nullptr || !model->HasTextureCoordinates() ||
            !model->SetTextureDimensions(static_cast<uint32_t>(texture->get_Width()), static_cast<uint32_t>(texture->get_Height()))) {
            throw new InvalidOperationException("N64 textured meshes require complete finite UVs that fit Tiny3D's signed 10.5 texture coordinates.");
        }
        texture->Bind();
    }

    ::Entity* entity = drawable->get_Parent();
    const ::float3 position = entity->get_Position();
    const ::float3 scale = entity->get_Scale();
    const ::float4 orientation = entity->get_Orientation();
    const float authoredScale[3] = {static_cast<float>(scale.X / 64.0f), static_cast<float>(scale.Y / 64.0f), static_cast<float>(scale.Z / 64.0f)};
    const float authoredRotation[4] = {static_cast<float>(orientation.X), static_cast<float>(orientation.Y), static_cast<float>(orientation.Z), static_cast<float>(orientation.W)};
    const float authoredPosition[3] = {static_cast<float>(position.X), static_cast<float>(position.Y), static_cast<float>(position.Z)};
    T3DMat4 modelMatrix;
    t3d_mat4_from_srt(&modelMatrix, authoredScale, authoredRotation, authoredPosition);
    Renderer->DrawAuthoredMesh(model->Vertices(), model->VertexCount(), model->Indices(), model->IndexCount(), modelMatrix, doubleSided, lit, FrameLights,
        texture == nullptr ? nullptr : texture->Surface());
    DrawCallCount++;
    if (!HasLoggedAuthoredDraw) {
        ::ModelAsset* rawModel = model->get_RawModelAsset();
        const HeCppString modelId = rawModel == nullptr ? HeCppString("<unknown>") : rawModel->get_Id();
        debugf("n64:first-authored-draw model=%s material=n64-standard lit=%d lights=%d calls=%ld\n", modelId.c_str(), lit ? 1 : 0, FrameLights.DirectionalCount, static_cast<long>(DrawCallCount));
        HasLoggedAuthoredDraw = true;
    }
}

} // namespace helengine::n64

#endif
