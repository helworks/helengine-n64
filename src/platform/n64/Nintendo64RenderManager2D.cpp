#include "platform/n64/Nintendo64RenderManager2D.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <libdragon.h>

#include "AssetSerializer.hpp"
#include "Core.hpp"
#include "ICamera.hpp"
#include "IContentStreamSource.hpp"
#include "IRenderQueue2D.hpp"
#include "ObjectManager.hpp"
#include "RenderCommandList2D.hpp"
#include "RuntimeTexture.hpp"
#include "TextureAsset.hpp"
#include "runtime/array.hpp"
#include "runtime/native_cast.hpp"
#include "runtime/native_exceptions.hpp"
#include "runtime/native_list.hpp"
#include "system/io/stream.hpp"

#include "platform/n64/Nintendo64RuntimeTexture.hpp"

namespace helengine::n64 {
namespace {

/// Reads one cooked asset through the content source and deserializes it.  This mirrors the helper in
/// `Nintendo64RenderManager3D.cpp`: the two managers deliberately keep their own copy rather than share a
/// header, because the only thing they have in common is these four lines.
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

Nintendo64RenderManager2D::~Nintendo64RenderManager2D() {
    if (CommandListBuilder != nullptr) {
        CommandListBuilder->Dispose();
        delete CommandListBuilder;
    }
}

::RuntimeTexture* Nintendo64RenderManager2D::BuildTextureFromRaw(::TextureAsset* data) {
    if (data == nullptr) {
        throw new ArgumentNullException("data");
    }
    Nintendo64RuntimeTexture* texture = new Nintendo64RuntimeTexture();
    if (!texture->Initialize(data)) {
        delete texture;
        throw new InvalidOperationException("N64 textures require a straight RGBA cooked payload.");
    }
    debugf("n64:texture-loaded id=%s size=%ldx%ld\n", data->get_Id().c_str(), static_cast<long>(texture->get_Width()), static_cast<long>(texture->get_Height()));
    return texture;
}

::RuntimeTexture* Nintendo64RenderManager2D::BuildTextureFromCooked(
    std::string cookedAssetPath, ::IContentStreamSource* contentStreamSource) {
    ::Asset* rawAsset = Deserialize(contentStreamSource, cookedAssetPath);
    ::TextureAsset* textureAsset = he_cpp_try_cast<::TextureAsset>(rawAsset);
    if (textureAsset == nullptr) {
        delete rawAsset;
        throw new InvalidOperationException("N64 cooked texture asset has the wrong type.");
    }
    // The runtime texture copies every texel into its own surface, so the cooked payload is dead weight once
    // the build returns.  Dispose frees the pixel array, which the defaulted asset destructor would otherwise
    // leak.  BuildTextureFromRaw throws on an unusable payload, so the release has to happen on that path too
    // rather than only on the one that returns.
    ::RuntimeTexture* runtimeTexture = nullptr;
    try {
        runtimeTexture = BuildTextureFromRaw(textureAsset);
    } catch (...) {
        textureAsset->Dispose();
        delete textureAsset;
        throw;
    }
    textureAsset->Dispose();
    delete textureAsset;
    return runtimeTexture;
}

void Nintendo64RenderManager2D::Draw() {
    ::Core* core = get_OwnerCore();
    if (core == nullptr || core->get_ObjectManager() == nullptr) {
        return;
    }
    ::List<::ICamera*>* cameras = core->get_ObjectManager()->get_Cameras();
    if (cameras == nullptr || cameras->get_Count() <= 0) {
        return;
    }
    if (CommandListBuilder == nullptr) {
        CommandListBuilder = new ::RenderCommandListBuilder2D();
    }

    // The 3D pass leaves the RDP configured for shaded, z-buffered Tiny3D triangles.  Wait for that pipeline
    // to drain, then reset the render mode from scratch: rdpq_set_mode_standard clears z-compare, z-write,
    // antialiasing, blending, fog and dithering, which is exactly the screen-space overlay state the 2D
    // commands need.  Nothing leaks back the other way, because Tiny3D's t3d_frame_start reissues its own
    // full mode reset at the top of every 3D pass.
    rdpq_sync_pipe();
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_alphacompare(1);
    rdpq_mode_filter(FILTER_POINT);

    const int32_t frameWidth = static_cast<int32_t>(display_get_width());
    const int32_t frameHeight = static_cast<int32_t>(display_get_height());
    for (int32_t cameraIndex = 0; cameraIndex < cameras->get_Count(); ++cameraIndex) {
        ::ICamera* camera = (*cameras)[cameraIndex];
        if (camera == nullptr || camera->get_RenderQueue2D() == nullptr) {
            continue;
        }
        // The builder owns and reuses this list across frames.  Deleting it here would free the builder's
        // own state and leave the next frame reading freed memory.
        ::RenderCommandList2D* commandList = CommandListBuilder->Build(camera->get_RenderQueue2D());
        if (commandList == nullptr || commandList->get_Count() <= 0) {
            continue;
        }
        Executor.Execute(commandList, frameWidth, frameHeight);
    }
}

void Nintendo64RenderManager2D::DrawRoundedRect(::IRoundedRectDrawable2D*) {
    // Generated Core reaches these only when something visits the queue with this manager as the visitor.
    // The N64 pass builds a command list instead, so arriving here means the pass was driven the old way
    // and every drawable would be silently dropped - the same defect class as the dead 2D pass shipped in
    // the sprite step.  Fail where it can be seen.
    debugf("n64:error unexpected-drawable-dispatch\n");
    throw new NotSupportedException("N64 draws its 2D pass from a command list, not from drawable dispatch.");
}

void Nintendo64RenderManager2D::DrawSprite(::ISpriteDrawable2D*) {
    debugf("n64:error unexpected-drawable-dispatch\n");
    throw new NotSupportedException("N64 draws its 2D pass from a command list, not from drawable dispatch.");
}

void Nintendo64RenderManager2D::DrawText(::ITextDrawable2D*) {
    debugf("n64:error unexpected-drawable-dispatch\n");
    throw new NotSupportedException("N64 draws its 2D pass from a command list, not from drawable dispatch.");
}

void Nintendo64RenderManager2D::UpdateTextureRegionCore(::RuntimeTexture*, int32_t, int32_t, int32_t, int32_t,
                                                        ::Array<uint8_t>*, int32_t) {
    throw new NotSupportedException("N64 authored scenes do not update textures.");
}

} // namespace helengine::n64

#endif
