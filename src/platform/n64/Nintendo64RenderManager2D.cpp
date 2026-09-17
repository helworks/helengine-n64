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
#include "IRoundedRectDrawable2D.hpp"
#include "ISpriteDrawable2D.hpp"
#include "ObjectManager.hpp"
#include "RoundedRectGeometryBuilder.hpp"
#include "RuntimeTexture.hpp"
#include "TextureAsset.hpp"
#include "byte4.hpp"
#include "float2.hpp"
#include "float3.hpp"
#include "float4.hpp"
#include "int2.hpp"
#include "runtime/array.hpp"
#include "runtime/native_cast.hpp"
#include "runtime/native_exceptions.hpp"
#include "runtime/native_list.hpp"
#include "system/io/stream.hpp"

#include "platform/n64/Nintendo64RoundedRectGeometry.hpp"
#include "platform/n64/Nintendo64RuntimeTexture.hpp"
#include "platform/n64/Nintendo64SpriteGeometry.hpp"

namespace helengine::n64 {
namespace {

/// Number of equal-angle segments the rounded-rect fill ring is split into.  This matches the eight segments
/// per corner the DirectX 11 renderer uses, so a shape authored against the desktop preview keeps the same
/// silhouette here; at 32 triangles per rectangle it is also well inside what the RDP swallows for the
/// handful of overlay shapes an authored N64 scene carries.
constexpr int32_t kRoundedRectRingSteps = 32;

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

void Nintendo64RenderManager2D::DrawRoundedRect(::IRoundedRectDrawable2D* shape) {
    if (shape == nullptr || shape->get_Parent() == nullptr) {
        return;
    }
    ::Entity* parent = shape->get_Parent();
    if (shape->get_Rotation() != 0.0f) {
        // Same reasoning as the sprite path: an unhandled native exception inside the frame loop stops the
        // ROM with nothing on the debug channel, so name the reason before going down.  The fill ring is
        // built in screen space and has nowhere to carry a rotation, and drawing it unrotated would be
        // silently wrong.
        debugf("n64:error rounded-rect-rotation\n");
        throw new NotSupportedException("N64 rounded rectangles do not support rotation.");
    }

    // The host-tested seam owns every number that decides the shape; this function only reads the authored
    // component fields and turns the resolved ring into RDP calls.
    const ::int2 authoredSize = shape->get_Size();
    const ::float3 scale = parent->get_Scale();
    const Nintendo64RoundedRectFillGeometry geometry = Nintendo64RoundedRectGeometry::Resolve(
        authoredSize.X, authoredSize.Y, shape->get_Radius(), scale.X, scale.Y);
    if (geometry.Kind == Nintendo64RoundedRectGeometryKind::EmptyDestination) {
        // Trace it once, for the same reason the sprite path does: from an emulator log a silently skipped
        // rectangle is indistinguishable from one that never reached the render queue at all.
        if (!HasLoggedSkippedRoundedRect) {
            debugf("n64:first-rounded-rect-skipped reason=empty-size size=%ldx%ld\n",
                   static_cast<long>(Nintendo64RoundedRectGeometry::RoundToInt(geometry.DestinationWidth)),
                   static_cast<long>(Nintendo64RoundedRectGeometry::RoundToInt(geometry.DestinationHeight)));
            HasLoggedSkippedRoundedRect = true;
        }
        return;
    }

    // The authored position is the top-left corner, but the ring builder works from the centre.  Round the
    // corner onto a whole pixel exactly as the sprite blit does, so a rectangle and an icon authored on the
    // same reference-canvas grid still line up once the viewport downscale leaves them on half pixels.
    const ::float3 position = parent->get_Position();
    const float left = static_cast<float>(Nintendo64RoundedRectGeometry::RoundToInt(position.X));
    const float top = static_cast<float>(Nintendo64RoundedRectGeometry::RoundToInt(position.Y));
    ::RoundedRectGeometryBuilder builder;
    ::Array<::float2>* vertices = builder.BuildFillRingVertices(
        kRoundedRectRingSteps,
        geometry.DestinationWidth,
        geometry.DestinationHeight,
        geometry.Radius,
        left + geometry.DestinationWidth * 0.5f,
        top + geometry.DestinationHeight * 0.5f);
    if (vertices == nullptr) {
        throw new InvalidOperationException("N64 rounded-rect fill ring builder returned no vertices.");
    }

    // The 2D pass leaves the combiner on RDPQ_COMBINER_TEX_FLAT for sprites, which samples TEX0 and would
    // read whatever tile the last blit happened to leave loaded.  TRIFMT_FILL supplies position only, so the
    // triangles need the flat combiner that outputs the primitive colour and nothing else.  The queue is
    // ordered, a sprite can follow a rectangle, and every state change here has to be undone before
    // returning - hence the restore below rather than a reset at the top of DrawSprite.
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    const ::byte4 fillColor = shape->get_FillColor();
    rdpq_set_prim_color(RGBA32(fillColor.X, fillColor.Y, fillColor.Z, fillColor.W));

    // BuildFillRingVertices returns one triangle per ring segment, laid out as centre, boundary, boundary.
    const int32_t vertexCount = vertices->Length;
    for (int32_t index = 0; index + 2 < vertexCount; index += 3) {
        const ::float2& center = (*vertices)[index];
        const ::float2& first = (*vertices)[index + 1];
        const ::float2& second = (*vertices)[index + 2];
        const float centerVertex[2] = {center.X, center.Y};
        const float firstVertex[2] = {first.X, first.Y};
        const float secondVertex[2] = {second.X, second.Y};
        rdpq_triangle(&TRIFMT_FILL, centerVertex, firstVertex, secondVertex);
    }
    delete vertices;

    // Hand the pipeline back exactly as it was found, so a sprite queued after this rectangle blits with the
    // textured combiner the 2D pass set up rather than a flat primitive colour.
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);

    // DrawCallCount deliberately stays a sprite-blit counter.  It is reported by n64:first-sprite-draw, and
    // counting rectangles into it would make that line's value depend on the render order of unrelated
    // overlay shapes rather than on the sprites it describes.
    if (!HasLoggedRoundedRectDraw) {
        debugf("n64:first-rounded-rect-draw size=%ldx%ld radius=%ld/100 verts=%ld\n",
               static_cast<long>(Nintendo64RoundedRectGeometry::RoundToInt(geometry.DestinationWidth)),
               static_cast<long>(Nintendo64RoundedRectGeometry::RoundToInt(geometry.DestinationHeight)),
               static_cast<long>(Nintendo64RoundedRectGeometry::RoundToInt(geometry.Radius * 100.0f)),
               static_cast<long>(vertexCount));
        HasLoggedRoundedRectDraw = true;
    }
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
        // This throw fires from inside the frame loop, where an unhandled native exception stops the ROM dead
        // with nothing on the debug channel to say why.  Every other hard failure on this platform prints an
        // "n64:error <reason>" line first, so emit one here too before the ROM goes down.
        debugf("n64:error sprite-rotation\n");
        throw new NotSupportedException("N64 sprites do not support rotation.");
    }

    // Every number the blit needs comes out of the host-tested geometry seam; this function only turns the
    // authored component fields into scalars and turns the result into RDP calls.
    const ::float4 sourceRect = sprite->get_SourceRect();
    const ::int2 authoredSize = sprite->get_Size();
    const ::float3 scale = parent->get_Scale();
    const Nintendo64SpriteBlitGeometry geometry = Nintendo64SpriteGeometry::Resolve(
        texture->get_Width(),
        texture->get_Height(),
        sourceRect.X,
        sourceRect.Y,
        sourceRect.Z,
        sourceRect.W,
        authoredSize.X,
        authoredSize.Y,
        scale.X,
        scale.Y);
    if (geometry.Kind == Nintendo64SpriteGeometryKind::EmptySource) {
        return;
    }
    if (geometry.Kind == Nintendo64SpriteGeometryKind::EmptyDestination) {
        // A degenerate destination covers no pixels.  Skipping it here is deliberate: rdpq treats a zero
        // scale factor as 1.0, so passing it through would draw the sprite at full size instead of nothing.
        // Trace it once, because from an emulator log a silently skipped sprite is indistinguishable from a
        // sprite that never reached the render queue at all.
        if (!HasLoggedSkippedSprite) {
            debugf("n64:first-sprite-skipped reason=empty-destination size=%ldx%ld\n",
                   static_cast<long>(Nintendo64SpriteGeometry::RoundToInt(geometry.DestinationWidth)),
                   static_cast<long>(Nintendo64SpriteGeometry::RoundToInt(geometry.DestinationHeight)));
            HasLoggedSkippedSprite = true;
        }
        return;
    }

    const ::byte4 color = sprite->get_Color();
    rdpq_set_prim_color(RGBA32(color.X, color.Y, color.Z, color.W));

    const ::float3 position = parent->get_Position();
    rdpq_blitparms_t parms{};
    parms.s0 = geometry.SourceX;
    parms.t0 = geometry.SourceY;
    parms.width = geometry.SourceWidth;
    parms.height = geometry.SourceHeight;
    parms.scale_x = geometry.ScaleX;
    parms.scale_y = geometry.ScaleY;
    rdpq_tex_blit(texture->Surface(),
                  static_cast<float>(Nintendo64SpriteGeometry::RoundToInt(position.X)),
                  static_cast<float>(Nintendo64SpriteGeometry::RoundToInt(position.Y)),
                  &parms);

    DrawCallCount++;
    if (!HasLoggedSpriteDraw) {
        debugf("n64:first-sprite-draw size=%ldx%ld calls=%ld\n",
               static_cast<long>(Nintendo64SpriteGeometry::RoundToInt(geometry.DestinationWidth)),
               static_cast<long>(Nintendo64SpriteGeometry::RoundToInt(geometry.DestinationHeight)),
               static_cast<long>(DrawCallCount));
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
