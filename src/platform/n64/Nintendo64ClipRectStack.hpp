#pragma once

#include <cstdint>

namespace helengine::n64 {

/// One resolved clip rectangle in whole frame-buffer pixels, already intersected with everything enclosing
/// it.  `IsEmpty` is carried separately rather than inferred from a zero extent so a caller cannot
/// accidentally hand a negative width to the RDP scissor.
struct Nintendo64ClipRect {
    /// Left pixel of the clip rectangle.
    int32_t X;
    /// Top pixel of the clip rectangle.
    int32_t Y;
    /// Width of the clip rectangle in pixels; zero when the rectangle is empty.
    int32_t Width;
    /// Height of the clip rectangle in pixels; zero when the rectangle is empty.
    int32_t Height;
    /// True when the rectangle covers no pixels and everything inside it must be skipped.
    bool IsEmpty;
};

/// Fixed-depth stack that resolves the shared command list's `ClipPush` and `ClipPop` commands into RDP
/// scissor rectangles.  It allocates nothing: a frame's clip nesting on this platform is a handful of
/// levels, and a heap allocation inside the frame loop is not worth the generality.
class Nintendo64ClipRectStack {
public:
    /// Deepest nesting the stack accepts before `Push` refuses and throws.  Public, and not merely an
    /// implementation detail, because a caller has to be able to check it before pushing: comparing
    /// `Depth()` against this limit lets the caller report an overflow on its own terms - with whatever
    /// trace its platform requires - rather than letting `Push`'s bare `std::length_error` escape unannounced.
    static constexpr int32_t MaximumDepth = 16;

    /// Builds one stack whose base rectangle is the whole frame.
    /// <param name="frameWidth">Frame-buffer width in pixels.</param>
    /// <param name="frameHeight">Frame-buffer height in pixels.</param>
    Nintendo64ClipRectStack(int32_t frameWidth, int32_t frameHeight);

    /// Pushes one clip rectangle, intersected with the enclosing rectangle and with the frame.
    /// <param name="x">Left edge in frame-buffer pixels.</param>
    /// <param name="y">Top edge in frame-buffer pixels.</param>
    /// <param name="width">Width in frame-buffer pixels.</param>
    /// <param name="height">Height in frame-buffer pixels.</param>
    void Push(float x, float y, float width, float height);

    /// Pops the innermost clip rectangle, restoring the one that encloses it.
    void Pop();

    /// Gets the innermost resolved clip rectangle, or the whole frame when nothing is pushed.
    /// <returns>Innermost clip rectangle.</returns>
    Nintendo64ClipRect Current() const;

    /// Gets how many rectangles are currently pushed.
    /// <returns>Current nesting depth.</returns>
    int32_t Depth() const;

private:
    /// Clip rectangle covering the whole frame, used when nothing is pushed.
    Nintendo64ClipRect FrameRect;
    /// Resolved rectangles, one per pushed level.
    Nintendo64ClipRect Levels[MaximumDepth];
    /// Number of rectangles currently pushed.
    int32_t DepthValue;
};

} // namespace helengine::n64
