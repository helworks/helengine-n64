#include "Nintendo64SpriteGeometry.hpp"

#include <cmath>

namespace helengine::n64 {

/// Rounds one float to the nearest whole pixel or texel.
int32_t Nintendo64SpriteGeometry::RoundToInt(float value) {
    return static_cast<int32_t>(std::lround(value));
}

/// Clamps one integer into the inclusive range [minimum, maximum].
int32_t Nintendo64SpriteGeometry::ClampToRange(int32_t value, int32_t minimum, int32_t maximum) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

/// Resolves one authored sprite against the texture it samples.
Nintendo64SpriteBlitGeometry Nintendo64SpriteGeometry::Resolve(
    int32_t textureWidth,
    int32_t textureHeight,
    float sourceRectX,
    float sourceRectY,
    float sourceRectWidth,
    float sourceRectHeight,
    int32_t authoredWidth,
    int32_t authoredHeight,
    float parentScaleX,
    float parentScaleY) {
    Nintendo64SpriteBlitGeometry geometry;
    if (textureWidth <= 0 || textureHeight <= 0) {
        geometry.Kind = Nintendo64SpriteGeometryKind::EmptySource;
        return geometry;
    }

    // A source rect with a positive extent selects a normalized sub-rect of the texture; anything else means
    // the sprite was authored without one and covers the whole texture.
    int32_t sourceX = 0;
    int32_t sourceY = 0;
    int32_t sourceWidth = textureWidth;
    int32_t sourceHeight = textureHeight;
    if (sourceRectWidth > 0.0f && sourceRectHeight > 0.0f) {
        sourceX = RoundToInt(sourceRectX * static_cast<float>(textureWidth));
        sourceY = RoundToInt(sourceRectY * static_cast<float>(textureHeight));
        sourceWidth = RoundToInt(sourceRectWidth * static_cast<float>(textureWidth));
        sourceHeight = RoundToInt(sourceRectHeight * static_cast<float>(textureHeight));
    }
    sourceX = ClampToRange(sourceX, 0, textureWidth);
    sourceY = ClampToRange(sourceY, 0, textureHeight);
    sourceWidth = ClampToRange(sourceWidth, 0, textureWidth - sourceX);
    sourceHeight = ClampToRange(sourceHeight, 0, textureHeight - sourceY);

    geometry.SourceX = sourceX;
    geometry.SourceY = sourceY;
    geometry.SourceWidth = sourceWidth;
    geometry.SourceHeight = sourceHeight;
    if (sourceWidth <= 0 || sourceHeight <= 0) {
        geometry.Kind = Nintendo64SpriteGeometryKind::EmptySource;
        return geometry;
    }

    // An authored size overrides the texel extent; a missing one means the sprite is drawn at texel size.
    // Either way the parent scale multiplies it, and the blitter wants that as a ratio, not a pixel count.
    const bool hasAuthoredSize = authoredWidth > 0 && authoredHeight > 0;
    const float destinationWidth = static_cast<float>(hasAuthoredSize ? authoredWidth : sourceWidth) * parentScaleX;
    const float destinationHeight = static_cast<float>(hasAuthoredSize ? authoredHeight : sourceHeight) * parentScaleY;
    geometry.DestinationWidth = destinationWidth;
    geometry.DestinationHeight = destinationHeight;
    if (destinationWidth <= 0.0f || destinationHeight <= 0.0f) {
        geometry.Kind = Nintendo64SpriteGeometryKind::EmptyDestination;
        return geometry;
    }

    geometry.ScaleX = destinationWidth / static_cast<float>(sourceWidth);
    geometry.ScaleY = destinationHeight / static_cast<float>(sourceHeight);
    geometry.Kind = Nintendo64SpriteGeometryKind::Drawable;
    return geometry;
}

} // namespace helengine::n64
