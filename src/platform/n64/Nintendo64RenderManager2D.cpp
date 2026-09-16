#include "platform/n64/Nintendo64RenderManager2D.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cmath>

#include <libdragon.h>

#include "AssetSerializer.hpp"
#include "Core.hpp"
#include "Entity.hpp"
#include "ICamera.hpp"
#include "IContentStreamSource.hpp"
#include "IDrawable2D.hpp"
#include "IRenderQueue2D.hpp"
#include "ISpriteDrawable2D.hpp"
#include "ObjectManager.hpp"
#include "RuntimeTexture.hpp"
#include "TextureAsset.hpp"
#include "byte4.hpp"
#include "float3.hpp"
#include "float4.hpp"
#include "int2.hpp"
#include "runtime/array.hpp"
#include "runtime/native_cast.hpp"
#include "runtime/native_exceptions.hpp"
#include "runtime/native_list.hpp"
#include "system/io/stream.hpp"

#include "platform/n64/Nintendo64RuntimeTexture.hpp"

namespace helengine::n64 {
namespace {

/// Tolerance used when deciding whether an entity orientation is the identity quaternion.  Authored
/// quaternions arrive as cooked floats, so an exact comparison would reject sprites that are visually
/// unrotated; a tolerance this wide still rejects any rotation the RDP blitter could not reproduce.
constexpr float kOrientationEpsilon = 1e-4f;

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

/// Rounds one float to the nearest whole pixel or texel.  Sprite placement and sub-rects arrive as floats but
/// the RDP addresses texels and screen pixels as integers, so every conversion goes through here.
int32_t RoundToInt(float value) {
    return static_cast<int32_t>(std::lround(value));
}

/// Clamps one integer into the inclusive range [minimum, maximum].  Used to keep a cooked source sub-rect
/// inside the texture it addresses so the blitter never reads past the surface.
int32_t ClampToRange(int32_t value, int32_t minimum, int32_t maximum) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

/// True when the entity orientation is the identity quaternion within `kOrientationEpsilon`.  The 2D pass
/// draws axis-aligned textured rectangles, so a rotated sprite has to be rejected loudly rather than drawn
/// unrotated and silently wrong.  Both quaternion signs describe the identity rotation, hence the |W| test.
bool IsIdentityOrientation(const ::float4& orientation) {
    return std::fabs(orientation.X) <= kOrientationEpsilon &&
           std::fabs(orientation.Y) <= kOrientationEpsilon &&
           std::fabs(orientation.Z) <= kOrientationEpsilon &&
           std::fabs(std::fabs(orientation.W) - 1.0f) <= kOrientationEpsilon;
}

} // namespace

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
    ::RuntimeTexture* runtimeTexture = BuildTextureFromRaw(textureAsset);
    // The runtime texture copies every texel into its own surface, so the cooked payload is dead weight from
    // here on.  Dispose frees the pixel array, which the defaulted asset destructor would otherwise leak.
    textureAsset->Dispose();
    delete textureAsset;
    return runtimeTexture;
}

void Nintendo64RenderManager2D::Draw() {
    DrawCallCount = 0;
    ::Core* core = get_OwnerCore();
    if (core == nullptr || core->get_ObjectManager() == nullptr) {
        return;
    }
    ::List<::ICamera*>* cameras = core->get_ObjectManager()->get_Cameras();
    if (cameras == nullptr || cameras->get_Count() <= 0) {
        return;
    }
    ::ICamera* camera = (*cameras)[0];
    if (camera == nullptr || camera->get_RenderQueue2D() == nullptr) {
        return;
    }

    // The 3D pass leaves the RDP configured for shaded, z-buffered Tiny3D triangles.  Wait for that pipeline
    // to drain, then reset the render mode from scratch: rdpq_set_mode_standard clears z-compare, z-write,
    // antialiasing, blending, fog and dithering, which is exactly the screen-space overlay state sprites
    // need.  Nothing leaks back the other way, because Tiny3D's t3d_frame_start reissues its own full mode
    // reset at the top of every 3D pass.
    rdpq_sync_pipe();
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
    rdpq_mode_alphacompare(1);
    rdpq_mode_filter(FILTER_POINT);
    camera->get_RenderQueue2D()->VisitOrdered(this);
}

void Nintendo64RenderManager2D::DrawRoundedRect(::IRoundedRectDrawable2D*) {
}

void Nintendo64RenderManager2D::DrawSprite(::ISpriteDrawable2D* sprite) {
    if (sprite == nullptr || sprite->get_Parent() == nullptr || sprite->get_Texture() == nullptr) {
        return;
    }
    Nintendo64RuntimeTexture* texture = he_cpp_try_cast<Nintendo64RuntimeTexture>(sprite->get_Texture());
    if (texture == nullptr || texture->Surface() == nullptr) {
        return;
    }
    ::Entity* parent = sprite->get_Parent();
    if (!IsIdentityOrientation(parent->get_Orientation())) {
        throw new NotSupportedException("N64 sprites do not support rotation.");
    }

    const int32_t textureWidth = texture->get_Width();
    const int32_t textureHeight = texture->get_Height();
    if (textureWidth <= 0 || textureHeight <= 0) {
        return;
    }

    // A source rect with a positive extent selects a normalized sub-rect of the texture; anything else means
    // the sprite was authored without one and covers the whole texture.
    const ::float4 sourceRect = sprite->get_SourceRect();
    int32_t sourceX = 0;
    int32_t sourceY = 0;
    int32_t sourceWidth = textureWidth;
    int32_t sourceHeight = textureHeight;
    if (sourceRect.Z > 0.0f && sourceRect.W > 0.0f) {
        sourceX = RoundToInt(sourceRect.X * static_cast<float>(textureWidth));
        sourceY = RoundToInt(sourceRect.Y * static_cast<float>(textureHeight));
        sourceWidth = RoundToInt(sourceRect.Z * static_cast<float>(textureWidth));
        sourceHeight = RoundToInt(sourceRect.W * static_cast<float>(textureHeight));
    }
    sourceX = ClampToRange(sourceX, 0, textureWidth);
    sourceY = ClampToRange(sourceY, 0, textureHeight);
    sourceWidth = ClampToRange(sourceWidth, 0, textureWidth - sourceX);
    sourceHeight = ClampToRange(sourceHeight, 0, textureHeight - sourceY);
    if (sourceWidth <= 0 || sourceHeight <= 0) {
        return;
    }

    // An authored size overrides the texel extent; a missing one means the sprite is drawn at texel size.
    // Either way the parent scale multiplies it, and the blitter wants that as a ratio, not a pixel count.
    const ::int2 authoredSize = sprite->get_Size();
    const bool hasAuthoredSize = authoredSize.X > 0 && authoredSize.Y > 0;
    const ::float3 scale = parent->get_Scale();
    const float destinationWidth = static_cast<float>(hasAuthoredSize ? authoredSize.X : sourceWidth) * scale.X;
    const float destinationHeight = static_cast<float>(hasAuthoredSize ? authoredSize.Y : sourceHeight) * scale.Y;
    if (destinationWidth <= 0.0f || destinationHeight <= 0.0f) {
        // A degenerate destination covers no pixels.  Skipping it here is deliberate: rdpq treats a zero
        // scale factor as 1.0, so passing it through would draw the sprite at full size instead of nothing.
        return;
    }
    const float scaleX = destinationWidth / static_cast<float>(sourceWidth);
    const float scaleY = destinationHeight / static_cast<float>(sourceHeight);

    const ::byte4 color = sprite->get_Color();
    rdpq_set_prim_color(RGBA32(color.X, color.Y, color.Z, color.W));

    const ::float3 position = parent->get_Position();
    rdpq_blitparms_t parms{};
    parms.s0 = sourceX;
    parms.t0 = sourceY;
    parms.width = sourceWidth;
    parms.height = sourceHeight;
    parms.scale_x = scaleX;
    parms.scale_y = scaleY;
    rdpq_tex_blit(texture->Surface(), static_cast<float>(RoundToInt(position.X)), static_cast<float>(RoundToInt(position.Y)), &parms);

    DrawCallCount++;
    if (!HasLoggedSpriteDraw) {
        debugf("n64:first-sprite-draw size=%ldx%ld calls=%ld\n", static_cast<long>(RoundToInt(destinationWidth)), static_cast<long>(RoundToInt(destinationHeight)), static_cast<long>(DrawCallCount));
        HasLoggedSpriteDraw = true;
    }
}

void Nintendo64RenderManager2D::DrawText(::ITextDrawable2D*) {
}

void Nintendo64RenderManager2D::Visit(::IDrawable2D* drawable) {
    if (drawable == nullptr) {
        return;
    }
    drawable->Draw();
}

void Nintendo64RenderManager2D::UpdateTextureRegionCore(::RuntimeTexture*, int32_t, int32_t, int32_t, int32_t,
                                                        ::Array<uint8_t>*, int32_t) {
    throw new NotSupportedException("N64 authored scenes do not update textures.");
}

} // namespace helengine::n64

#endif
