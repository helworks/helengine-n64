#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cstdint>

#include "RenderCommandList2D.hpp"

namespace helengine::n64 {

/// Executes one flattened 2D command list against the RDP.  The shared `RenderCommandListBuilder2D` has
/// already resolved every drawable into clip pushes, textured quads, glyph quads and rounded rectangles in
/// frame-buffer pixels, so this class contains no layout logic at all: it reads commands, resolves their
/// geometry through the pure seams, and issues rdpq calls.
class Nintendo64RenderCommandExecutor2D {
public:
    /// Executes every command in one list.
    /// <param name="commandList">List produced by the shared builder; never owned or freed here.</param>
    /// <param name="frameWidth">Frame-buffer width in pixels, used to clamp clip rectangles.</param>
    /// <param name="frameHeight">Frame-buffer height in pixels, used to clamp clip rectangles.</param>
    void Execute(::RenderCommandList2D* commandList, int32_t frameWidth, int32_t frameHeight);

    /// Gets how many quads the most recent `Execute` blitted, counting textured and glyph quads alike.
    /// <returns>Quad count of the most recent execution.</returns>
    int32_t DrawCallCount() const;

private:
    /// Blits one textured or glyph quad.
    /// <param name="texture">Runtime texture the quad samples.</param>
    /// <param name="bounds">Destination rectangle in frame-buffer pixels.</param>
    /// <param name="sourceRect">Normalized source rectangle into the texture.</param>
    /// <param name="color">Primitive colour the flat combiner modulates the texel with.</param>
    /// <param name="isGlyph">True when the quad came from a glyph command, which changes only tracing.</param>
    void BlitQuad(::RuntimeTexture* texture, ::float4 bounds, ::float4 sourceRect, ::byte4 color, bool isGlyph);

    /// Fills one rounded rectangle as a flat-coloured triangle fan and restores the textured combiner.
    /// <param name="commandList">List the command was read from.</param>
    /// <param name="payloadIndex">Payload index of the rounded-rect command.</param>
    void FillRoundedRect(::RenderCommandList2D* commandList, int32_t payloadIndex);

    /// Quads blitted during the most recent execution.
    int32_t DrawCallCountValue = 0;
    /// True once the first sprite blit of the run has been traced.
    bool HasLoggedSpriteDraw = false;
    /// True once the first glyph blit of the run has been traced.
    bool HasLoggedGlyphDraw = false;
    /// True once the first quad skipped for a zero-area destination has been traced.
    bool HasLoggedSkippedQuad = false;
    /// True once the first rounded-rectangle fill of the run has been traced.
    bool HasLoggedRoundedRectDraw = false;
};

} // namespace helengine::n64

#endif
