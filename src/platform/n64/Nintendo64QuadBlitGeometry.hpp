#pragma once

#include <cstdint>

namespace helengine::n64 {

/// Outcome kind of one resolved quad blit.  Two of the three outcomes cover no pixels, and they are kept
/// apart because a caller wants to trace them differently: an empty source usually means a bad source rect,
/// an empty destination usually means an authored size or scale of zero.
enum class Nintendo64QuadBlitKind {
    /// The quad covers at least one texel and at least one destination pixel.
    Drawable,
    /// The source sub-rect covers no texels, so there is nothing to sample.
    EmptySource,
    /// The destination rectangle covers no pixels, so there is nowhere to draw.
    EmptyDestination
};

/// Everything `rdpq_tex_blit` needs to draw one quad from the shared 2D command list, resolved from a
/// normalized source rectangle and a destination rectangle already expressed in frame-buffer pixels.
struct Nintendo64QuadBlitGeometry2D {
    /// Outcome kind; the remaining fields are meaningful only when this is `Drawable`.
    Nintendo64QuadBlitKind Kind;
    /// Left texel of the source sub-rect, cropped to the texture.
    int32_t SourceX;
    /// Top texel of the source sub-rect, cropped to the texture.
    int32_t SourceY;
    /// Width of the source sub-rect in texels, cropped to the texture.
    int32_t SourceWidth;
    /// Height of the source sub-rect in texels, cropped to the texture.
    int32_t SourceHeight;
    /// Left frame-buffer pixel the quad is drawn at.
    int32_t DestinationX;
    /// Top frame-buffer pixel the quad is drawn at.
    int32_t DestinationY;
    /// Destination width in frame-buffer pixels, kept fractional for tracing.
    float DestinationWidth;
    /// Destination height in frame-buffer pixels, kept fractional for tracing.
    float DestinationHeight;
    /// Horizontal blit scale, the destination width divided by the sampled texel width.
    float ScaleX;
    /// Vertical blit scale, the destination height divided by the sampled texel height.
    float ScaleY;
};

/// Center-based placement required by libdragon when a textured quad is rotated.
struct Nintendo64QuadBlitRotation2D {
    float AnchorX;
    float AnchorY;
    int32_t CenterX;
    int32_t CenterY;
    float RotationRadians;
};

/// Pure arithmetic that turns one command-list quad into blit parameters.  Sprites and glyphs both reach
/// the RDP through this, because the shared command list has already reduced them to the same shape: a
/// destination rectangle in pixels and a normalized source rectangle into one texture.
class Nintendo64QuadBlitGeometry {
public:
    /// Resolves one quad blit.
    /// <param name="textureWidth">Texture width in texels.</param>
    /// <param name="textureHeight">Texture height in texels.</param>
    /// <param name="sourceRectX">Normalized left edge of the source sub-rect.</param>
    /// <param name="sourceRectY">Normalized top edge of the source sub-rect.</param>
    /// <param name="sourceRectWidth">Normalized width of the source sub-rect; a non-positive value selects
    /// the whole texture, which is how an authored sprite with no source rect arrives.</param>
    /// <param name="sourceRectHeight">Normalized height of the source sub-rect; a non-positive value
    /// selects the whole texture.</param>
    /// <param name="boundsX">Destination left edge in frame-buffer pixels.</param>
    /// <param name="boundsY">Destination top edge in frame-buffer pixels.</param>
    /// <param name="boundsWidth">Destination width in frame-buffer pixels.</param>
    /// <param name="boundsHeight">Destination height in frame-buffer pixels.</param>
    /// <returns>Resolved blit geometry and its outcome kind.</returns>
    static Nintendo64QuadBlitGeometry2D Resolve(int32_t textureWidth, int32_t textureHeight,
                                                float sourceRectX, float sourceRectY,
                                                float sourceRectWidth, float sourceRectHeight,
                                                float boundsX, float boundsY,
                                                float boundsWidth, float boundsHeight);

    /// Resolves the transformed source hotspot while preserving the command's world-space top-left and orientation.
    static Nintendo64QuadBlitRotation2D ResolveRotation(const Nintendo64QuadBlitGeometry2D& geometry,
                                                        float rotationRadians);

    /// True only for a positive, exactly axis-aligned TL/TR/BR/BL quad.
    static bool IsAxisAligned(const float (&x)[4], const float (&y)[4]);

    /// Builds libdragon's column-major source-pixel-to-screen matrix without angle/scale decomposition.
    /// Returns false for nonfinite or zero-area corners and nonpositive source extents.
    static bool TryResolveAffineTransform(const float (&x)[4], const float (&y)[4],
        int32_t sourceWidth, int32_t sourceHeight, float (&matrix)[3][3]);

    /// Rounds one coordinate to the nearest integer, halves away from zero.
    /// <param name="value">Value to round.</param>
    /// <returns>Nearest integer.</returns>
    static int32_t RoundToInt(float value);
};

} // namespace helengine::n64
