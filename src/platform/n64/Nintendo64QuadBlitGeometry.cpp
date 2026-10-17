#include "src/platform/n64/Nintendo64QuadBlitGeometry.hpp"

#include <cmath>

namespace helengine::n64 {

bool Nintendo64QuadBlitGeometry::IsAxisAligned(const float (&x)[4], const float (&y)[4]) {
    return y[0] == y[1] && y[2] == y[3] && x[0] == x[3] && x[1] == x[2]
        && x[1] > x[0] && y[3] > y[0];
}

bool Nintendo64QuadBlitGeometry::TryResolveAffineTransform(const float (&x)[4], const float (&y)[4],
    int32_t sourceWidth, int32_t sourceHeight, float (&matrix)[3][3]) {
    if (sourceWidth <= 0 || sourceHeight <= 0) return false;
    for (unsigned i = 0; i < 4; ++i) {
        if (!std::isfinite(x[i]) || !std::isfinite(y[i])) return false;
    }
    const float rightX = x[1] - x[0], rightY = y[1] - y[0];
    const float downX = x[3] - x[0], downY = y[3] - y[0];
    const float area = rightX * downY - rightY * downX;
    if (!std::isfinite(area) || area == 0.0f) return false;
    matrix[0][0] = rightX / sourceWidth; matrix[0][1] = rightY / sourceWidth; matrix[0][2] = 0;
    matrix[1][0] = downX / sourceHeight; matrix[1][1] = downY / sourceHeight; matrix[1][2] = 0;
    matrix[2][0] = x[0]; matrix[2][1] = y[0]; matrix[2][2] = 1;
    return true;
}

int32_t Nintendo64QuadBlitGeometry::RoundToInt(float value) {
    return static_cast<int32_t>(std::round(value));
}

Nintendo64QuadBlitRotation2D Nintendo64QuadBlitGeometry::ResolveRotation(
    const Nintendo64QuadBlitGeometry2D& geometry, float rotationRadians) {
    const int32_t sourceCenterX = geometry.SourceWidth / 2;
    const int32_t sourceCenterY = geometry.SourceHeight / 2;
    const float centerOffsetX = sourceCenterX * geometry.ScaleX;
    const float centerOffsetY = sourceCenterY * geometry.ScaleY;
    const float cosine = std::cos(rotationRadians);
    const float sine = std::sin(rotationRadians);
    // The command's position is already the transformed top-left. libdragon
    // places the source hotspot at the supplied screen anchor, and its positive
    // theta rotates counter-clockwise in screen space.
    return Nintendo64QuadBlitRotation2D{
        static_cast<float>(geometry.DestinationX) + cosine * centerOffsetX - sine * centerOffsetY,
        static_cast<float>(geometry.DestinationY) + sine * centerOffsetX + cosine * centerOffsetY,
        sourceCenterX,
        sourceCenterY,
        -rotationRadians};
}

Nintendo64QuadBlitGeometry2D Nintendo64QuadBlitGeometry::Resolve(int32_t textureWidth, int32_t textureHeight,
                                                                 float sourceRectX, float sourceRectY,
                                                                 float sourceRectWidth, float sourceRectHeight,
                                                                 float boundsX, float boundsY,
                                                                 float boundsWidth, float boundsHeight) {
    Nintendo64QuadBlitGeometry2D geometry{};
    geometry.Kind = Nintendo64QuadBlitKind::EmptySource;
    geometry.DestinationX = RoundToInt(boundsX);
    geometry.DestinationY = RoundToInt(boundsY);
    geometry.DestinationWidth = boundsWidth;
    geometry.DestinationHeight = boundsHeight;

    if (textureWidth <= 0 || textureHeight <= 0) {
        return geometry;
    }

    // A sprite authored without a source rect arrives as four zeros.  Treating that as the whole texture is
    // the documented fallback, and it is applied per axis so a rect that sets only one axis still works.
    float normalizedX = sourceRectX;
    float normalizedWidth = sourceRectWidth;
    if (normalizedWidth <= 0.0f) {
        normalizedX = 0.0f;
        normalizedWidth = 1.0f;
    }
    float normalizedY = sourceRectY;
    float normalizedHeight = sourceRectHeight;
    if (normalizedHeight <= 0.0f) {
        normalizedY = 0.0f;
        normalizedHeight = 1.0f;
    }

    int32_t sourceX = RoundToInt(normalizedX * static_cast<float>(textureWidth));
    int32_t sourceY = RoundToInt(normalizedY * static_cast<float>(textureHeight));
    int32_t sourceWidth = RoundToInt(normalizedWidth * static_cast<float>(textureWidth));
    int32_t sourceHeight = RoundToInt(normalizedHeight * static_cast<float>(textureHeight));

    // Crop rather than shift.  A sub-rect that starts before the near edge is missing those texels; moving
    // it to zero instead would sample a different glyph entirely.
    if (sourceX < 0) {
        sourceWidth += sourceX;
        sourceX = 0;
    }
    if (sourceY < 0) {
        sourceHeight += sourceY;
        sourceY = 0;
    }
    if (sourceX >= textureWidth || sourceY >= textureHeight) {
        return geometry;
    }
    if (sourceX + sourceWidth > textureWidth) {
        sourceWidth = textureWidth - sourceX;
    }
    if (sourceY + sourceHeight > textureHeight) {
        sourceHeight = textureHeight - sourceY;
    }
    if (sourceWidth <= 0 || sourceHeight <= 0) {
        return geometry;
    }

    geometry.SourceX = sourceX;
    geometry.SourceY = sourceY;
    geometry.SourceWidth = sourceWidth;
    geometry.SourceHeight = sourceHeight;

    if (boundsWidth <= 0.0f || boundsHeight <= 0.0f) {
        geometry.Kind = Nintendo64QuadBlitKind::EmptyDestination;
        return geometry;
    }

    geometry.ScaleX = boundsWidth / static_cast<float>(sourceWidth);
    geometry.ScaleY = boundsHeight / static_cast<float>(sourceHeight);
    geometry.Kind = Nintendo64QuadBlitKind::Drawable;
    return geometry;
}

} // namespace helengine::n64
