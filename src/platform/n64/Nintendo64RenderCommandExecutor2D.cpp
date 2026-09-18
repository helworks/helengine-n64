#include "platform/n64/Nintendo64RenderCommandExecutor2D.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <libdragon.h>

#include "RenderCommand2DType.hpp"
#include "RoundedRectGeometryBuilder.hpp"
#include "RuntimeTexture.hpp"
#include "byte4.hpp"
#include "float2.hpp"
#include "float4.hpp"
#include "runtime/array.hpp"
#include "runtime/native_cast.hpp"
#include "runtime/native_exceptions.hpp"

#include "platform/n64/Nintendo64ClipRectStack.hpp"
#include "platform/n64/Nintendo64QuadBlitGeometry.hpp"
#include "platform/n64/Nintendo64RoundedRectGeometry.hpp"
#include "platform/n64/Nintendo64RuntimeTexture.hpp"

namespace helengine::n64 {
namespace {

/// Number of equal-angle segments the rounded-rect fill ring is split into; unchanged from the step that
/// introduced it, so an authored shape keeps the same silhouette.
constexpr int32_t kRoundedRectRingSteps = 32;

} // namespace

int32_t Nintendo64RenderCommandExecutor2D::DrawCallCount() const {
    return DrawCallCountValue;
}

void Nintendo64RenderCommandExecutor2D::Execute(::RenderCommandList2D* commandList, int32_t frameWidth,
                                                int32_t frameHeight) {
    if (commandList == nullptr) {
        return;
    }
    DrawCallCountValue = 0;

    // Every hard failure raised inside the frame loop has to announce itself on the debug channel first: an
    // unhandled native exception here stops the ROM dead with nothing else to explain why.  The clip stack
    // constructor throws `std::invalid_argument` on a non-positive dimension, which is a bare native
    // exception type with no trace of its own, so the frame size is validated here instead of letting that
    // escape unannounced.
    if (frameWidth <= 0 || frameHeight <= 0) {
        debugf("n64:error invalid-frame-size\n");
        throw new InvalidOperationException("N64 2D executor requires a positive frame size.");
    }
    Nintendo64ClipRectStack clipStack(frameWidth, frameHeight);

    const int32_t commandCount = commandList->get_Count();
    for (int32_t commandIndex = 0; commandIndex < commandCount; ++commandIndex) {
        const ::RenderCommand2DType commandType = commandList->GetCommandType(commandIndex);
        if (commandType == ::RenderCommand2DType::ClipPush) {
            const ::float4 clipRect = commandList->GetClipPushRect(commandList->GetClipPushPayloadIndex(commandIndex));
            clipStack.Push(clipRect.X, clipRect.Y, clipRect.Z, clipRect.W);
            const Nintendo64ClipRect current = clipStack.Current();
            rdpq_set_scissor(current.X, current.Y, current.X + current.Width, current.Y + current.Height);
            continue;
        }
        if (commandType == ::RenderCommand2DType::ClipPop) {
            clipStack.Pop();
            const Nintendo64ClipRect current = clipStack.Current();
            rdpq_set_scissor(current.X, current.Y, current.X + current.Width, current.Y + current.Height);
            continue;
        }
        if (clipStack.Current().IsEmpty) {
            // Everything inside an empty clip covers no pixels; skipping here saves the RDP the work.
            continue;
        }
        if (commandType == ::RenderCommand2DType::TexturedQuad) {
            const int32_t payloadIndex = commandList->GetTexturedQuadPayloadIndex(commandIndex);
            const float rotationRadians = commandList->GetTexturedQuadRotation(payloadIndex);
            if (rotationRadians != 0.0f) {
                debugf("n64:error quad-rotation\n");
                throw new NotSupportedException("N64 2D quads do not support rotation.");
            }
            BlitQuad(commandList->GetTexturedQuadTexture(payloadIndex),
                     commandList->GetTexturedQuadBounds(payloadIndex),
                     commandList->GetTexturedQuadSourceRect(payloadIndex),
                     commandList->GetTexturedQuadColor(payloadIndex),
                     false);
            continue;
        }
        if (commandType == ::RenderCommand2DType::GlyphQuad) {
            const int32_t payloadIndex = commandList->GetGlyphQuadPayloadIndex(commandIndex);
            BlitQuad(commandList->GetGlyphQuadTexture(payloadIndex),
                     commandList->GetGlyphQuadBounds(payloadIndex),
                     commandList->GetGlyphQuadSourceRect(payloadIndex),
                     commandList->GetGlyphQuadColor(payloadIndex),
                     true);
            continue;
        }
        if (commandType == ::RenderCommand2DType::RoundedRect) {
            FillRoundedRect(commandList, commandList->GetRoundedRectPayloadIndex(commandIndex));
        }
    }

    // Leave the scissor covering the whole frame, because the 3D pass of the next frame does not set it.
    rdpq_set_scissor(0, 0, frameWidth, frameHeight);
}

void Nintendo64RenderCommandExecutor2D::BlitQuad(::RuntimeTexture* texture, ::float4 bounds, ::float4 sourceRect,
                                                 ::byte4 color, bool isGlyph) {
    if (texture == nullptr) {
        if (isGlyph) {
            // A glyph with no atlas is a hard content error: every character on screen would be blank and
            // nothing else on the debug channel would explain why, so name it before going down.
            debugf("n64:error glyph-missing-atlas\n");
            throw new InvalidOperationException("N64 glyph quad has no texture atlas.");
        }
        // The shared builder already drops sprites with no texture before they reach the command list, so a
        // textured quad reaching here with a null texture means the queue changed under us mid-frame.  There
        // is nothing to draw and nothing more specific to report.
        return;
    }

    Nintendo64RuntimeTexture* concreteTexture = he_cpp_try_cast<Nintendo64RuntimeTexture>(texture);
    if (concreteTexture == nullptr || concreteTexture->Surface() == nullptr) {
        return;
    }

    // Every number the blit needs comes out of the host-tested geometry seam; this function only turns the
    // command's fields into scalars and turns the result into RDP calls.
    const Nintendo64QuadBlitGeometry2D geometry = Nintendo64QuadBlitGeometry::Resolve(
        concreteTexture->get_Width(), concreteTexture->get_Height(),
        sourceRect.X, sourceRect.Y, sourceRect.Z, sourceRect.W,
        bounds.X, bounds.Y, bounds.Z, bounds.W);
    if (geometry.Kind == Nintendo64QuadBlitKind::EmptySource) {
        return;
    }
    if (geometry.Kind == Nintendo64QuadBlitKind::EmptyDestination) {
        // A degenerate destination covers no pixels.  Trace it once, because from an emulator log a silently
        // skipped quad is indistinguishable from one that never reached the command list at all.
        if (!HasLoggedSkippedQuad) {
            debugf("n64:first-quad-skipped reason=empty-destination\n");
            HasLoggedSkippedQuad = true;
        }
        return;
    }

    rdpq_set_prim_color(RGBA32(color.X, color.Y, color.Z, color.W));

    rdpq_blitparms_t parms{};
    parms.s0 = geometry.SourceX;
    parms.t0 = geometry.SourceY;
    parms.width = geometry.SourceWidth;
    parms.height = geometry.SourceHeight;
    parms.scale_x = geometry.ScaleX;
    parms.scale_y = geometry.ScaleY;
    rdpq_tex_blit(concreteTexture->Surface(), static_cast<float>(geometry.DestinationX),
                  static_cast<float>(geometry.DestinationY), &parms);

    ++DrawCallCountValue;
    if (isGlyph) {
        if (!HasLoggedGlyphDraw) {
            debugf("n64:first-glyph-draw size=%ldx%ld calls=%ld\n",
                   static_cast<long>(Nintendo64QuadBlitGeometry::RoundToInt(geometry.DestinationWidth)),
                   static_cast<long>(Nintendo64QuadBlitGeometry::RoundToInt(geometry.DestinationHeight)),
                   static_cast<long>(DrawCallCountValue));
            HasLoggedGlyphDraw = true;
        }
    } else if (!HasLoggedSpriteDraw) {
        debugf("n64:first-sprite-draw size=%ldx%ld calls=%ld\n",
               static_cast<long>(Nintendo64QuadBlitGeometry::RoundToInt(geometry.DestinationWidth)),
               static_cast<long>(Nintendo64QuadBlitGeometry::RoundToInt(geometry.DestinationHeight)),
               static_cast<long>(DrawCallCountValue));
        HasLoggedSpriteDraw = true;
    }
}

void Nintendo64RenderCommandExecutor2D::FillRoundedRect(::RenderCommandList2D* commandList, int32_t payloadIndex) {
    const ::float4 bounds = commandList->GetRoundedRectBounds(payloadIndex);
    const float radius = commandList->GetRoundedRectRadius(payloadIndex);
    const ::byte4 fillColor = commandList->GetRoundedRectFillColor(payloadIndex);

    // The command list exposes only the resolved bounds, radius and fill colour for this command - no parent
    // scale - so this always resolves against a unit scale rather than the entity scale the old drawable-
    // dispatch path used to read directly off the parent.
    const Nintendo64RoundedRectFillGeometry geometry = Nintendo64RoundedRectGeometry::Resolve(
        Nintendo64RoundedRectGeometry::RoundToInt(bounds.Z),
        Nintendo64RoundedRectGeometry::RoundToInt(bounds.W),
        radius,
        1.0f,
        1.0f);
    if (geometry.Kind == Nintendo64RoundedRectGeometryKind::EmptyDestination) {
        // Trace it once, for the same reason a skipped quad is traced: from an emulator log a silently
        // skipped rectangle is indistinguishable from one that never reached the command list at all.
        if (!HasLoggedSkippedQuad) {
            debugf("n64:first-quad-skipped reason=empty-destination\n");
            HasLoggedSkippedQuad = true;
        }
        return;
    }

    // The command's position is the top-left corner, but the ring builder works from the centre.  Round the
    // corner onto a whole pixel exactly as the quad blit does, so a rectangle and an icon authored on the
    // same reference-canvas grid still line up once the viewport downscale leaves them on half pixels.
    const float left = static_cast<float>(Nintendo64RoundedRectGeometry::RoundToInt(bounds.X));
    const float top = static_cast<float>(Nintendo64RoundedRectGeometry::RoundToInt(bounds.Y));
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

    // The 2D pass leaves the combiner on RDPQ_COMBINER_TEX_FLAT for quads, which samples TEX0 and would read
    // whatever tile the last blit happened to leave loaded.  TRIFMT_FILL supplies position only, so the
    // triangles need the flat combiner that outputs the primitive colour and nothing else.  The command list
    // is ordered, a quad can follow a rectangle, and every state change here has to be undone before
    // returning - hence the restore below rather than a reset at the top of BlitQuad.
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
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

    // Hand the pipeline back exactly as it was found, so a quad queued after this rectangle blits with the
    // textured combiner the 2D pass set up rather than a flat primitive colour.
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);

    if (!HasLoggedRoundedRectDraw) {
        debugf("n64:first-rounded-rect-draw size=%ldx%ld radius=%ld/100 verts=%ld\n",
               static_cast<long>(Nintendo64RoundedRectGeometry::RoundToInt(geometry.DestinationWidth)),
               static_cast<long>(Nintendo64RoundedRectGeometry::RoundToInt(geometry.DestinationHeight)),
               static_cast<long>(Nintendo64RoundedRectGeometry::RoundToInt(geometry.Radius * 100.0f)),
               static_cast<long>(vertexCount));
        HasLoggedRoundedRectDraw = true;
    }
}

} // namespace helengine::n64

#endif
