#include <cmath>
#include <cstdint>
#include <cstdio>

#include "src/platform/n64/Nintendo64QuadBlitGeometry.hpp"

namespace {

using helengine::n64::Nintendo64QuadBlitGeometry;
using helengine::n64::Nintendo64QuadBlitGeometry2D;
using helengine::n64::Nintendo64QuadBlitKind;

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

bool NearlyEqual(float actual, float expected) {
    return std::fabs(actual - expected) <= 1e-4f;
}

/// A glyph's normalized source rect denormalizes onto real texels, and the destination bounds arrive
/// already in pixels, so the scale factors are the ratio of the two.
void TestGlyphSubRectDenormalizesOntoTexels() {
    const Nintendo64QuadBlitGeometry2D geometry =
        Nintendo64QuadBlitGeometry::Resolve(256, 128, 0.25f, 0.5f, 0.125f, 0.25f, 40.0f, 12.0f, 16.0f, 16.0f);

    Check(geometry.Kind == Nintendo64QuadBlitKind::Drawable, "a glyph sub-rect is drawable");
    Check(geometry.SourceX == 64, "0.25 of a 256 texel atlas is texel 64");
    Check(geometry.SourceY == 64, "0.5 of a 128 texel atlas is texel 64");
    Check(geometry.SourceWidth == 32, "0.125 of 256 texels is 32 texels");
    Check(geometry.SourceHeight == 32, "0.25 of 128 texels is 32 texels");
    Check(geometry.DestinationX == 40, "the destination origin is the authored bounds origin");
    Check(geometry.DestinationY == 12, "the destination origin is the authored bounds origin");
    Check(NearlyEqual(geometry.ScaleX, 0.5f), "16 destination pixels over 32 texels is a half scale");
    Check(NearlyEqual(geometry.ScaleY, 0.5f), "16 destination pixels over 32 texels is a half scale");
}

/// The two axes are independent. A square source drawn into a non-square destination must produce two
/// different scale factors, so an implementation that swapped X for Y fails here.
void TestAxesScaleIndependently() {
    const Nintendo64QuadBlitGeometry2D geometry =
        Nintendo64QuadBlitGeometry::Resolve(64, 64, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 64.0f, 16.0f);

    Check(geometry.SourceWidth == 32, "half of 64 texels is 32 texels");
    Check(geometry.SourceHeight == 32, "half of 64 texels is 32 texels");
    Check(NearlyEqual(geometry.ScaleX, 2.0f), "64 destination pixels over 32 texels is a double scale");
    Check(NearlyEqual(geometry.ScaleY, 0.5f), "16 destination pixels over 32 texels is a half scale");
}

/// A sub-rect that starts before the near edge is cropped, not shifted. Shifting it would draw the wrong
/// glyph; cropping draws the part of it that exists.
void TestSubRectBeforeTheNearEdgeIsCropped() {
    const Nintendo64QuadBlitGeometry2D geometry =
        Nintendo64QuadBlitGeometry::Resolve(64, 64, -0.25f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 32.0f, 64.0f);

    Check(geometry.SourceX == 0, "a sub-rect starting off the near edge begins at texel zero");
    Check(geometry.SourceWidth == 16, "the 16 texels that fell off the near edge are dropped, not shifted");
}

/// A sub-rect running past the far edge is clamped to the texels that exist.
void TestSubRectPastTheFarEdgeIsClamped() {
    const Nintendo64QuadBlitGeometry2D geometry =
        Nintendo64QuadBlitGeometry::Resolve(64, 64, 0.75f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 32.0f, 64.0f);

    Check(geometry.SourceX == 48, "0.75 of 64 texels is texel 48");
    Check(geometry.SourceWidth == 16, "only the 16 texels up to the far edge survive");
}

/// A source rect with no extent means the whole texture. Sprites authored without a source rect rely on it.
void TestMissingSourceRectFallsBackToTheWholeTexture() {
    const Nintendo64QuadBlitGeometry2D geometry =
        Nintendo64QuadBlitGeometry::Resolve(32, 16, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 32.0f, 16.0f);

    Check(geometry.Kind == Nintendo64QuadBlitKind::Drawable, "a missing source rect is drawable");
    Check(geometry.SourceWidth == 32, "the fallback covers the whole texture width");
    Check(geometry.SourceHeight == 16, "the fallback covers the whole texture height");
}

/// A sub-rect entirely off the texture covers no texels and must be reported as an empty source rather
/// than drawn at some clamped size.
void TestSubRectEntirelyOffTheTextureReportsEmptySource() {
    const Nintendo64QuadBlitGeometry2D geometry =
        Nintendo64QuadBlitGeometry::Resolve(64, 64, 1.5f, 0.0f, 0.25f, 1.0f, 0.0f, 0.0f, 16.0f, 64.0f);

    Check(geometry.Kind == Nintendo64QuadBlitKind::EmptySource, "a sub-rect past the texture is an empty source");
}

/// A destination with no area covers no pixels. It must be reported rather than blitted, because rdpq
/// treats a zero scale factor as 1.0 and would draw the quad at full size instead of nothing.
void TestZeroAreaDestinationReportsEmptyDestination() {
    const Nintendo64QuadBlitGeometry2D geometry =
        Nintendo64QuadBlitGeometry::Resolve(64, 64, 0.0f, 0.0f, 1.0f, 1.0f, 10.0f, 10.0f, 0.0f, 20.0f);

    Check(geometry.Kind == Nintendo64QuadBlitKind::EmptyDestination, "a zero width destination is empty");
}

/// A texture with no extent cannot be sampled at all.
void TestNonPositiveTextureSizeReportsEmptySource() {
    const Nintendo64QuadBlitGeometry2D geometry =
        Nintendo64QuadBlitGeometry::Resolve(0, 64, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 16.0f, 16.0f);

    Check(geometry.Kind == Nintendo64QuadBlitKind::EmptySource, "a zero width texture is an empty source");
}

/// Rounding is away from zero on halves, matching the rounding the other seams use.
void TestRoundToIntRoundsHalvesAway() {
    Check(Nintendo64QuadBlitGeometry::RoundToInt(7.5f) == 8, "7.5 rounds up");
    Check(Nintendo64QuadBlitGeometry::RoundToInt(7.4f) == 7, "7.4 rounds down");
    Check(Nintendo64QuadBlitGeometry::RoundToInt(-7.5f) == -8, "-7.5 rounds away from zero");
}

} // namespace

int main() {
    TestGlyphSubRectDenormalizesOntoTexels();
    TestAxesScaleIndependently();
    TestSubRectBeforeTheNearEdgeIsCropped();
    TestSubRectPastTheFarEdgeIsClamped();
    TestMissingSourceRectFallsBackToTheWholeTexture();
    TestSubRectEntirelyOffTheTextureReportsEmptySource();
    TestZeroAreaDestinationReportsEmptyDestination();
    TestNonPositiveTextureSizeReportsEmptySource();
    TestRoundToIntRoundsHalvesAway();
    if (failures != 0) {
        std::fprintf(stderr, "%d quad-blit-geometry test(s) failed\n", failures);
        return 1;
    }
    std::puts("n64-quad-blit-geometry-tests: PASS");
    return 0;
}
