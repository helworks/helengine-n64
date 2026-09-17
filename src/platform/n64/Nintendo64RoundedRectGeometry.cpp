#include "Nintendo64RoundedRectGeometry.hpp"

#include <cmath>

namespace helengine::n64 {
namespace {

/// Returns the smaller of two floats. The rounded-rect resolve needs a minimum in three places and the
/// repository style keeps helpers like this file-local rather than nested inside the function that uses them.
float SmallerOf(float first, float second) {
    return first < second ? first : second;
}

} // namespace

/// Rounds one float to the nearest whole pixel.
int32_t Nintendo64RoundedRectGeometry::RoundToInt(float value) {
    return static_cast<int32_t>(std::lround(value));
}

/// Resolves one authored rounded rectangle against the scale of the entity that carries it.
Nintendo64RoundedRectFillGeometry Nintendo64RoundedRectGeometry::Resolve(
    int32_t authoredWidth,
    int32_t authoredHeight,
    float authoredRadius,
    float parentScaleX,
    float parentScaleY) {
    Nintendo64RoundedRectFillGeometry geometry;

    const float destinationWidth = static_cast<float>(authoredWidth) * parentScaleX;
    const float destinationHeight = static_cast<float>(authoredHeight) * parentScaleY;
    geometry.DestinationWidth = destinationWidth;
    geometry.DestinationHeight = destinationHeight;
    if (destinationWidth <= 0.0f || destinationHeight <= 0.0f) {
        // A rectangle with no area on either axis covers no pixels, and the fill ring builder would emit a
        // fan of degenerate triangles around a point.  Report it so the caller can take its traced skip path
        // instead of handing the RDP work that draws nothing.
        geometry.Kind = Nintendo64RoundedRectGeometryKind::EmptyDestination;
        return geometry;
    }

    // The authored radius is a single scalar describing a circular corner, but the two axes can be scaled by
    // different factors.  A circle under a non-uniform scale is an ellipse, which the fill ring builder cannot
    // express, so the radius follows the smaller of the two scales: that keeps the corner arc inside the
    // rectangle on both axes instead of bulging past the shorter one.
    const float radiusScale = SmallerOf(parentScaleX, parentScaleY);
    float radius = authoredRadius * radiusScale;
    if (radius < 0.0f) {
        // An authored negative radius has no geometric meaning; the corner is simply square.
        radius = 0.0f;
    }

    // A radius larger than half the smaller side would push the corner centres past each other and fold the
    // ring inside out.  Half the smaller side is the largest radius that still describes a shape, and it is
    // the stadium the author was asking for when they wrote something larger.
    const float radiusLimit = SmallerOf(destinationWidth, destinationHeight) * 0.5f;
    geometry.Radius = SmallerOf(radius, radiusLimit);
    geometry.Kind = Nintendo64RoundedRectGeometryKind::Drawable;
    return geometry;
}

} // namespace helengine::n64
