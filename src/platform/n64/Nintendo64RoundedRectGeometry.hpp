#pragma once

#include <cstdint>

namespace helengine::n64 {

/// Why one resolved rounded rectangle can or cannot be filled.
enum class Nintendo64RoundedRectGeometryKind {
    /// The rectangle covers a positive area on both axes, so the fill ring is worth building.
    Drawable,
    /// The rectangle has square corners and can use the RDP's direct rectangle primitive.
    AxisAlignedRectangle,
    /// The destination collapsed to nothing once the parent scale was applied, so the fill would cover no
    /// pixels at all.
    EmptyDestination
};

/// One authored rounded rectangle resolved into the screen-space numbers the fill ring builder needs.
struct Nintendo64RoundedRectFillGeometry {
    /// Whether the rectangle is drawable, or which way it collapsed.
    Nintendo64RoundedRectGeometryKind Kind = Nintendo64RoundedRectGeometryKind::EmptyDestination;
    /// Destination width in screen pixels, before any rounding to whole pixels.
    float DestinationWidth = 0.0f;
    /// Destination height in screen pixels, before any rounding to whole pixels.
    float DestinationHeight = 0.0f;
    /// Corner radius in screen pixels, already scaled and clamped to half the smaller resolved side. A value
    /// below one pixel is legitimate and simply produces corners that read as square once rasterized.
    float Radius = 0.0f;
};

/// Resolves authored rounded-rectangle fields into screen-space fill geometry. The class deliberately depends
/// on nothing but `<cstdint>` and the standard math helpers, so the host test suite can exercise the scale
/// application and the radius clamp without libdragon or the generated core.
class Nintendo64RoundedRectGeometry {
public:
    /// Rounds one float to the nearest whole pixel. The authored overlay is positioned in fractional
    /// reference-canvas units but the traced size is reported in whole pixels, so every report goes through
    /// here rather than truncating and losing half a pixel.
    static int32_t RoundToInt(float value);

    /// Resolves one authored rounded rectangle against the scale of the entity that carries it.
    /// <param name="authoredWidth">Authored width in reference-canvas pixels.</param>
    /// <param name="authoredHeight">Authored height in reference-canvas pixels.</param>
    /// <param name="authoredRadius">Authored corner radius in reference-canvas pixels.</param>
    /// <param name="parentScaleX">Horizontal scale of the owning entity.</param>
    /// <param name="parentScaleY">Vertical scale of the owning entity.</param>
    /// <returns>Resolved geometry; the caller must inspect <c>Kind</c> before building a fill ring.</returns>
    static Nintendo64RoundedRectFillGeometry Resolve(
        int32_t authoredWidth,
        int32_t authoredHeight,
        float authoredRadius,
        float parentScaleX,
        float parentScaleY);
};

} // namespace helengine::n64
