#include "src/platform/n64/Nintendo64ClipRectStack.hpp"

#include <cmath>
#include <stdexcept>

namespace helengine::n64 {

Nintendo64ClipRectStack::Nintendo64ClipRectStack(int32_t frameWidth, int32_t frameHeight)
    : FrameRect{}, Levels{}, DepthValue(0) {
    const int32_t width = frameWidth > 0 ? frameWidth : 0;
    const int32_t height = frameHeight > 0 ? frameHeight : 0;
    FrameRect.X = 0;
    FrameRect.Y = 0;
    FrameRect.Width = width;
    FrameRect.Height = height;
    FrameRect.IsEmpty = width <= 0 || height <= 0;
}

int32_t Nintendo64ClipRectStack::Depth() const {
    return DepthValue;
}

Nintendo64ClipRect Nintendo64ClipRectStack::Current() const {
    if (DepthValue <= 0) {
        return FrameRect;
    }
    return Levels[DepthValue - 1];
}

void Nintendo64ClipRectStack::Push(float x, float y, float width, float height) {
    if (DepthValue >= MaximumDepth) {
        throw std::length_error("N64 clip rectangles nest deeper than the executor supports.");
    }

    const Nintendo64ClipRect enclosing = Current();
    Nintendo64ClipRect pushed{};
    if (enclosing.IsEmpty) {
        // Nothing nested inside an empty clip can draw, so the intersection is settled without arithmetic.
        pushed.IsEmpty = true;
        Levels[DepthValue] = pushed;
        ++DepthValue;
        return;
    }

    const int32_t left = static_cast<int32_t>(std::round(x));
    const int32_t top = static_cast<int32_t>(std::round(y));
    const int32_t right = left + static_cast<int32_t>(std::round(width));
    const int32_t bottom = top + static_cast<int32_t>(std::round(height));

    const int32_t enclosingRight = enclosing.X + enclosing.Width;
    const int32_t enclosingBottom = enclosing.Y + enclosing.Height;
    const int32_t clampedLeft = left > enclosing.X ? left : enclosing.X;
    const int32_t clampedTop = top > enclosing.Y ? top : enclosing.Y;
    const int32_t clampedRight = right < enclosingRight ? right : enclosingRight;
    const int32_t clampedBottom = bottom < enclosingBottom ? bottom : enclosingBottom;

    if (clampedRight <= clampedLeft || clampedBottom <= clampedTop) {
        pushed.IsEmpty = true;
    } else {
        pushed.X = clampedLeft;
        pushed.Y = clampedTop;
        pushed.Width = clampedRight - clampedLeft;
        pushed.Height = clampedBottom - clampedTop;
        pushed.IsEmpty = false;
    }

    Levels[DepthValue] = pushed;
    ++DepthValue;
}

void Nintendo64ClipRectStack::Pop() {
    if (DepthValue <= 0) {
        throw std::underflow_error("N64 clip stack received more pops than pushes.");
    }
    --DepthValue;
}

} // namespace helengine::n64
