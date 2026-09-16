#pragma once

#include <cstdint>

namespace helengine::n64 {

/// Why one resolved sprite can or cannot be blitted.
enum class Nintendo64SpriteGeometryKind {
    /// The sprite covers at least one source texel and at least one destination pixel.
    Drawable,
    /// The source sub-rect collapsed to nothing once clamped into the texture, so there is no texel to read.
    EmptySource,
    /// The destination collapsed to nothing, so the blit would cover no pixels.
    EmptyDestination
};

/// One authored sprite resolved into the numbers an RDP blit needs.
struct Nintendo64SpriteBlitGeometry {
    /// Whether the sprite is drawable, and when it is not, which extent collapsed.
    Nintendo64SpriteGeometryKind Kind = Nintendo64SpriteGeometryKind::EmptySource;
    /// Left texel of the source sub-rect, already clamped inside the texture.
    int32_t SourceX = 0;
    /// Top texel of the source sub-rect, already clamped inside the texture.
    int32_t SourceY = 0;
    /// Width in texels of the source sub-rect, never reaching past the texture's right edge.
    int32_t SourceWidth = 0;
    /// Height in texels of the source sub-rect, never reaching past the texture's bottom edge.
    int32_t SourceHeight = 0;
    /// Destination width in screen pixels before rounding.
    float DestinationWidth = 0.0f;
    /// Destination height in screen pixels before rounding.
    float DestinationHeight = 0.0f;
    /// Destination-to-source width ratio the blitter wants; only meaningful when the sprite is drawable.
    float ScaleX = 0.0f;
    /// Destination-to-source height ratio the blitter wants; only meaningful when the sprite is drawable.
    float ScaleY = 0.0f;
};

/// Resolves authored sprite fields into RDP blit geometry. The class deliberately depends on nothing but
/// `<cstdint>` and the standard rounding helper, so the host test suite can exercise the source-rect
/// denormalize, the texture-bounds clamp, the authored-size selection and the blit ratios without libdragon
/// or the generated core.
class Nintendo64SpriteGeometry {
public:
    /// Rounds one float to the nearest whole pixel or texel. Sprite placement and sub-rects arrive as floats
    /// but the RDP addresses texels and screen pixels as integers, so every conversion goes through here.
    static int32_t RoundToInt(float value);

    /// Clamps one integer into the inclusive range [minimum, maximum].
    static int32_t ClampToRange(int32_t value, int32_t minimum, int32_t maximum);

    /// Resolves one authored sprite against the texture it samples.
    /// <param name="textureWidth">Width in texels of the sampled texture.</param>
    /// <param name="textureHeight">Height in texels of the sampled texture.</param>
    /// <param name="sourceRectX">Normalized left edge of the authored source sub-rect.</param>
    /// <param name="sourceRectY">Normalized top edge of the authored source sub-rect.</param>
    /// <param name="sourceRectWidth">Normalized width of the authored source sub-rect; a non-positive value
    /// means the sprite was authored without one and covers the whole texture.</param>
    /// <param name="sourceRectHeight">Normalized height of the authored source sub-rect; a non-positive value
    /// means the sprite was authored without one and covers the whole texture.</param>
    /// <param name="authoredWidth">Authored destination width in pixels, or a non-positive value to fall back
    /// to the source extent on both axes.</param>
    /// <param name="authoredHeight">Authored destination height in pixels, or a non-positive value to fall
    /// back to the source extent on both axes.</param>
    /// <param name="parentScaleX">Horizontal scale of the owning entity.</param>
    /// <param name="parentScaleY">Vertical scale of the owning entity.</param>
    /// <returns>Resolved geometry; the caller must inspect <c>Kind</c> before blitting.</returns>
    static Nintendo64SpriteBlitGeometry Resolve(
        int32_t textureWidth,
        int32_t textureHeight,
        float sourceRectX,
        float sourceRectY,
        float sourceRectWidth,
        float sourceRectHeight,
        int32_t authoredWidth,
        int32_t authoredHeight,
        float parentScaleX,
        float parentScaleY);
};

} // namespace helengine::n64
