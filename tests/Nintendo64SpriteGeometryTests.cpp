#include <cmath>
#include <cstdint>
#include <cstdio>

#include "src/platform/n64/Nintendo64SpriteGeometry.hpp"

namespace {

using helengine::n64::Nintendo64SpriteBlitGeometry;
using helengine::n64::Nintendo64SpriteGeometry;
using helengine::n64::Nintendo64SpriteGeometryKind;

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

/// The cooked cube_test icons carry a normalized sub-rect, so the denormalize has to land on real texels.
void TestNormalizedSourceRectResolvesToTexels() {
    // CameraIconPrimary as cooked: a 64x64 texture with SourceRect 0.140625, 0.140625, 0.72265625 square.
    const Nintendo64SpriteBlitGeometry geometry = Nintendo64SpriteGeometry::Resolve(
        64, 64, 0.140625f, 0.140625f, 0.72265625f, 0.72265625f, 48, 48, 1.0f, 1.0f);

    Check(geometry.Kind == Nintendo64SpriteGeometryKind::Drawable, "cooked icon geometry is drawable");
    Check(geometry.SourceX == 9, "0.140625 * 64 denormalizes to texel 9");
    Check(geometry.SourceY == 9, "0.140625 * 64 denormalizes to texel 9 vertically");
    Check(geometry.SourceWidth == 46, "0.72265625 * 64 denormalizes to 46 texels");
    Check(geometry.SourceHeight == 46, "0.72265625 * 64 denormalizes to 46 texels vertically");
    Check(NearlyEqual(geometry.DestinationWidth, 48.0f), "authored width survives a unit scale");
    Check(NearlyEqual(geometry.DestinationHeight, 48.0f), "authored height survives a unit scale");
    Check(NearlyEqual(geometry.ScaleX, 48.0f / 46.0f), "scale x is destination over source width");
    Check(NearlyEqual(geometry.ScaleY, 48.0f / 46.0f), "scale y is destination over source height");
}

/// A non-positive source extent means the sprite was authored without a sub-rect at all.
void TestMissingSourceRectCoversTheWholeTexture() {
    const Nintendo64SpriteBlitGeometry geometry =
        Nintendo64SpriteGeometry::Resolve(32, 16, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 1.0f, 1.0f);

    Check(geometry.Kind == Nintendo64SpriteGeometryKind::Drawable, "a whole-texture sprite is drawable");
    Check(geometry.SourceX == 0 && geometry.SourceY == 0, "a missing source rect starts at the origin");
    Check(geometry.SourceWidth == 32 && geometry.SourceHeight == 16, "a missing source rect covers the texture");
    Check(NearlyEqual(geometry.DestinationWidth, 32.0f), "the fallback destination is the source width");
    Check(NearlyEqual(geometry.DestinationHeight, 16.0f), "the fallback destination is the source height");
    Check(NearlyEqual(geometry.ScaleX, 1.0f) && NearlyEqual(geometry.ScaleY, 1.0f),
          "an unscaled whole-texture sprite blits one to one");
}

/// A source rect reaching past the texture must clamp, never hand the blitter texels off the surface.
void TestOutOfBoundsSourceRectClampsInsideTheTexture() {
    // Starts three quarters across and asks for a full width, so it would read 48 texels past the right edge.
    const Nintendo64SpriteBlitGeometry geometry =
        Nintendo64SpriteGeometry::Resolve(64, 64, 0.75f, 0.75f, 1.0f, 1.0f, 0, 0, 1.0f, 1.0f);

    Check(geometry.Kind == Nintendo64SpriteGeometryKind::Drawable, "a clamped sprite is still drawable");
    Check(geometry.SourceX == 48 && geometry.SourceY == 48, "the clamped origin stays where it was authored");
    Check(geometry.SourceWidth == 16, "the width clamps to what is left of the texture");
    Check(geometry.SourceHeight == 16, "the height clamps to what is left of the texture");
    Check(geometry.SourceX + geometry.SourceWidth <= 64, "the clamped rect never passes the right edge");
    Check(geometry.SourceY + geometry.SourceHeight <= 64, "the clamped rect never passes the bottom edge");

    // A rect that starts beyond the texture entirely has nothing left to read.
    const Nintendo64SpriteBlitGeometry offTexture =
        Nintendo64SpriteGeometry::Resolve(64, 64, 1.5f, 0.0f, 0.5f, 0.5f, 16, 16, 1.0f, 1.0f);
    Check(offTexture.Kind == Nintendo64SpriteGeometryKind::EmptySource,
          "a source rect entirely off the texture reports an empty source");
    Check(offTexture.SourceX == 64 && offTexture.SourceWidth == 0,
          "an off-texture origin clamps to the edge with no width");
}

/// A sub-rect that starts off the left or top edge must lose the off-texture band, not slide back inside.
void TestNegativeSourceRectOriginCropsRatherThanShifts() {
    // -0.25 * 64 is eight texels off the left and top edge; 0.5 * 64 asks for 32 texels in each direction.
    // Cropping leaves the 24 texels that are actually on the texture, starting at the origin.  Shifting would
    // have kept all 32 and read texels 0..31, which is a different part of the image than the author picked.
    const Nintendo64SpriteBlitGeometry geometry =
        Nintendo64SpriteGeometry::Resolve(64, 64, -0.125f, -0.125f, 0.5f, 0.5f, 0, 0, 1.0f, 1.0f);

    Check(geometry.Kind == Nintendo64SpriteGeometryKind::Drawable, "a cropped sprite is still drawable");
    Check(geometry.SourceX == 0 && geometry.SourceY == 0, "a negative origin clamps to the texture origin");
    Check(geometry.SourceWidth == 24, "the width loses the eight texels that were off the left edge");
    Check(geometry.SourceHeight == 24, "the height loses the eight texels that were off the top edge");

    // A sub-rect entirely off the left edge has nothing left once it is cropped.
    const Nintendo64SpriteBlitGeometry offTexture =
        Nintendo64SpriteGeometry::Resolve(64, 64, -1.5f, 0.0f, 0.5f, 0.5f, 16, 16, 1.0f, 1.0f);
    Check(offTexture.Kind == Nintendo64SpriteGeometryKind::EmptySource,
          "a source rect entirely off the left edge reports an empty source");
    Check(offTexture.SourceWidth == 0, "cropping a fully off-texture rect leaves no width");
}

/// The authored size overrides the texel extent on both axes, or is ignored on both when either axis is unset.
void TestAuthoredSizeOverridesTheSourceExtent() {
    const Nintendo64SpriteBlitGeometry authored =
        Nintendo64SpriteGeometry::Resolve(64, 64, 0.0f, 0.0f, 1.0f, 1.0f, 48, 24, 1.0f, 1.0f);
    Check(NearlyEqual(authored.DestinationWidth, 48.0f), "the authored width wins over the source width");
    Check(NearlyEqual(authored.DestinationHeight, 24.0f), "the authored height wins over the source height");
    Check(NearlyEqual(authored.ScaleX, 0.75f), "48 over 64 texels is a 0.75 blit ratio");
    Check(NearlyEqual(authored.ScaleY, 0.375f), "24 over 64 texels is a 0.375 blit ratio");

    // One non-positive axis falls back to the source extent on BOTH axes, not just the missing one.
    const Nintendo64SpriteBlitGeometry singleAxis =
        Nintendo64SpriteGeometry::Resolve(64, 64, 0.0f, 0.0f, 1.0f, 1.0f, 48, 0, 1.0f, 1.0f);
    Check(NearlyEqual(singleAxis.DestinationWidth, 64.0f),
          "a single-axis authored size falls back to the source width too");
    Check(NearlyEqual(singleAxis.DestinationHeight, 64.0f),
          "a single-axis authored size falls back to the source height");
}

/// The reference-canvas downscale the cube_test overlay relies on has to reach the blit ratios.
void TestParentScaleMultipliesTheDestination() {
    // The authored 48x48 icon on a reference canvas four times wider than the 320x240 frame buffer.
    const Nintendo64SpriteBlitGeometry geometry = Nintendo64SpriteGeometry::Resolve(
        64, 64, 0.140625f, 0.140625f, 0.72265625f, 0.72265625f, 48, 48, 0.25f, 0.25f);

    Check(geometry.Kind == Nintendo64SpriteGeometryKind::Drawable, "a downscaled icon is drawable");
    Check(NearlyEqual(geometry.DestinationWidth, 12.0f), "48 authored pixels at quarter scale is 12");
    Check(NearlyEqual(geometry.DestinationHeight, 12.0f), "48 authored pixels at quarter scale is 12");
    Check(Nintendo64SpriteGeometry::RoundToInt(geometry.DestinationWidth) == 12,
          "the traced destination rounds to 12");
    Check(NearlyEqual(geometry.ScaleX, 12.0f / 46.0f), "the blit ratio follows the downscaled destination");
}

/// A zero destination must report empty so the caller still takes its traced skip branch.
void TestZeroDestinationReportsEmptyDestination() {
    const Nintendo64SpriteBlitGeometry zeroScale =
        Nintendo64SpriteGeometry::Resolve(64, 64, 0.0f, 0.0f, 1.0f, 1.0f, 48, 48, 0.0f, 1.0f);
    Check(zeroScale.Kind == Nintendo64SpriteGeometryKind::EmptyDestination,
          "a zero parent scale reports an empty destination, not an empty source");
    Check(zeroScale.SourceWidth == 64 && zeroScale.SourceHeight == 64,
          "the source extent is still resolved so the skip trace can report it");
    Check(NearlyEqual(zeroScale.DestinationWidth, 0.0f), "the empty destination width is zero");
    Check(NearlyEqual(zeroScale.ScaleX, 0.0f) && NearlyEqual(zeroScale.ScaleY, 0.0f),
          "an empty destination leaves the blit ratios unset");

    const Nintendo64SpriteBlitGeometry negativeScale =
        Nintendo64SpriteGeometry::Resolve(64, 64, 0.0f, 0.0f, 1.0f, 1.0f, 48, 48, 1.0f, -2.0f);
    Check(negativeScale.Kind == Nintendo64SpriteGeometryKind::EmptyDestination,
          "a negative parent scale reports an empty destination");
}

/// A texture the runtime failed to size cannot be sampled at all.
void TestNonPositiveTextureSizeReportsEmptySource() {
    Check(Nintendo64SpriteGeometry::Resolve(0, 64, 0.0f, 0.0f, 1.0f, 1.0f, 8, 8, 1.0f, 1.0f).Kind ==
              Nintendo64SpriteGeometryKind::EmptySource,
          "a zero-width texture reports an empty source");
    Check(Nintendo64SpriteGeometry::Resolve(64, -1, 0.0f, 0.0f, 1.0f, 1.0f, 8, 8, 1.0f, 1.0f).Kind ==
              Nintendo64SpriteGeometryKind::EmptySource,
          "a negative-height texture reports an empty source");
}

void TestRoundToIntRoundsHalvesAway() {
    Check(Nintendo64SpriteGeometry::RoundToInt(11.5f) == 12, "11.5 rounds up");
    Check(Nintendo64SpriteGeometry::RoundToInt(11.4f) == 11, "11.4 rounds down");
    Check(Nintendo64SpriteGeometry::RoundToInt(-11.5f) == -12, "-11.5 rounds away from zero");
}

void TestClampToRangeHoldsTheBounds() {
    Check(Nintendo64SpriteGeometry::ClampToRange(-5, 0, 64) == 0, "a value below the range clamps up");
    Check(Nintendo64SpriteGeometry::ClampToRange(70, 0, 64) == 64, "a value above the range clamps down");
    Check(Nintendo64SpriteGeometry::ClampToRange(33, 0, 64) == 33, "a value inside the range is kept");
}

} // namespace

int main() {
    TestNormalizedSourceRectResolvesToTexels();
    TestMissingSourceRectCoversTheWholeTexture();
    TestOutOfBoundsSourceRectClampsInsideTheTexture();
    TestNegativeSourceRectOriginCropsRatherThanShifts();
    TestAuthoredSizeOverridesTheSourceExtent();
    TestParentScaleMultipliesTheDestination();
    TestZeroDestinationReportsEmptyDestination();
    TestNonPositiveTextureSizeReportsEmptySource();
    TestRoundToIntRoundsHalvesAway();
    TestClampToRangeHoldsTheBounds();
    if (failures != 0) {
        std::fprintf(stderr, "%d sprite-geometry test(s) failed\n", failures);
        return 1;
    }
    std::puts("n64-sprite-geometry-tests: PASS");
    return 0;
}
