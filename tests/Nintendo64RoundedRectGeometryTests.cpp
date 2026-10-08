#include <cmath>
#include <cstdint>
#include <cstdio>

#include "src/platform/n64/Nintendo64RoundedRectGeometry.hpp"

namespace {

using helengine::n64::Nintendo64RoundedRectFillGeometry;
using helengine::n64::Nintendo64RoundedRectGeometry;
using helengine::n64::Nintendo64RoundedRectGeometryKind;

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

/// At a unit scale the authored numbers reach the fill ring untouched, radius included.
void TestUnitScaleKeepsTheAuthoredNumbers() {
    const Nintendo64RoundedRectFillGeometry geometry =
        Nintendo64RoundedRectGeometry::Resolve(64, 48, 8.0f, 1.0f, 1.0f);

    Check(geometry.Kind == Nintendo64RoundedRectGeometryKind::Drawable, "a unit-scale rectangle is drawable");
    Check(NearlyEqual(geometry.DestinationWidth, 64.0f), "the authored width survives a unit scale");
    Check(NearlyEqual(geometry.DestinationHeight, 48.0f), "the authored height survives a unit scale");
    Check(NearlyEqual(geometry.Radius, 8.0f), "the authored radius survives a unit scale");
    // Half the smaller side is 24 pixels here, so the clamp cannot be what produced the 8 above.
    Check(geometry.Radius < geometry.DestinationHeight * 0.5f,
          "an 8 pixel radius is well inside the 24 pixel limit, so the clamp did not fire");
}

/// The parent scale multiplies both axes and the radius, because the whole overlay is authored on a
/// reference canvas the viewport shrinks onto the 320x240 frame buffer.
void TestParentScaleAppliesToBothAxesAndTheRadius() {
    const Nintendo64RoundedRectFillGeometry geometry =
        Nintendo64RoundedRectGeometry::Resolve(64, 48, 8.0f, 0.25f, 0.25f);

    Check(geometry.Kind == Nintendo64RoundedRectGeometryKind::Drawable, "a downscaled rectangle is drawable");
    Check(NearlyEqual(geometry.DestinationWidth, 16.0f), "64 authored pixels at quarter scale is 16");
    Check(NearlyEqual(geometry.DestinationHeight, 12.0f), "48 authored pixels at quarter scale is 12");
    Check(NearlyEqual(geometry.Radius, 2.0f), "an 8 pixel radius at quarter scale is 2");
    Check(Nintendo64RoundedRectGeometry::RoundToInt(geometry.DestinationWidth) == 16,
          "the traced destination width rounds to 16");
    Check(Nintendo64RoundedRectGeometry::RoundToInt(geometry.DestinationHeight) == 12,
          "the traced destination height rounds to 12");

    // A scale that differs per axis stretches the rectangle but the corner stays circular, so the radius has
    // to follow the smaller factor rather than either one at random.
    const Nintendo64RoundedRectFillGeometry stretched =
        Nintendo64RoundedRectGeometry::Resolve(64, 64, 8.0f, 2.0f, 0.5f);
    Check(NearlyEqual(stretched.DestinationWidth, 128.0f), "the horizontal scale reaches the width alone");
    Check(NearlyEqual(stretched.DestinationHeight, 32.0f), "the vertical scale reaches the height alone");
    Check(NearlyEqual(stretched.Radius, 4.0f), "the radius follows the smaller of the two scales");
}

/// An authored radius larger than the rectangle can hold folds the ring inside out, so it clamps.
void TestRadiusClampsToHalfTheSmallerSide() {
    // A 32x8 rectangle can hold a four pixel radius at most; the authored twenty is meaningless.
    const Nintendo64RoundedRectFillGeometry clamped =
        Nintendo64RoundedRectGeometry::Resolve(32, 8, 20.0f, 1.0f, 1.0f);
    Check(clamped.Kind == Nintendo64RoundedRectGeometryKind::Drawable, "an over-radiused rectangle is drawable");
    Check(NearlyEqual(clamped.Radius, 4.0f), "the radius clamps to half the smaller side, not half the larger");
    Check(NearlyEqual(clamped.DestinationWidth, 32.0f), "clamping the radius leaves the width alone");
    Check(NearlyEqual(clamped.DestinationHeight, 8.0f), "clamping the radius leaves the height alone");

    // The clamp is applied after the scale, so a radius that fits at unit scale still fits when both shrink
    // together, and one that does not still clamps.
    const Nintendo64RoundedRectFillGeometry scaledClamp =
        Nintendo64RoundedRectGeometry::Resolve(32, 8, 20.0f, 0.5f, 0.5f);
    Check(NearlyEqual(scaledClamp.Radius, 2.0f), "the scaled clamp is half the scaled smaller side");

    // A negative authored radius has no geometric meaning and resolves to a square corner.
    const Nintendo64RoundedRectFillGeometry negative =
        Nintendo64RoundedRectGeometry::Resolve(32, 32, -6.0f, 1.0f, 1.0f);
    Check(negative.Kind == Nintendo64RoundedRectGeometryKind::AxisAlignedRectangle,
          "a negative radius is drawable as a square rectangle");
    Check(NearlyEqual(negative.Radius, 0.0f), "a negative authored radius resolves to a square corner");
}

/// A zero-radius shape needs no triangle fan; the N64 can submit it as one direct rectangle primitive.
void TestZeroRadiusSelectsTheDirectRectanglePath() {
    const Nintendo64RoundedRectFillGeometry geometry =
        Nintendo64RoundedRectGeometry::Resolve(320, 1, 0.0f, 1.0f, 1.0f);

    Check(geometry.Kind == Nintendo64RoundedRectGeometryKind::AxisAlignedRectangle,
          "a zero-radius scanline selects the direct rectangle path");
    Check(NearlyEqual(geometry.Radius, 0.0f), "the direct rectangle path has no corner radius");
}

/// The cube_test swatch resolves to a radius well under one pixel, and that is a real shape, not an error.
void TestSubPixelRadiusStaysDrawable() {
    // DemoDiscLightIndicatorSwatch as authored: 32x32 with a radius of 2, on a 1280x720 reference canvas the
    // viewport shrinks by a factor of four.
    const Nintendo64RoundedRectFillGeometry geometry =
        Nintendo64RoundedRectGeometry::Resolve(32, 32, 2.0f, 0.25f, 0.25f);

    Check(geometry.Kind == Nintendo64RoundedRectGeometryKind::Drawable,
          "a sub-pixel radius is drawable, not an error");
    Check(NearlyEqual(geometry.DestinationWidth, 8.0f), "32 authored pixels at quarter scale is 8");
    Check(NearlyEqual(geometry.DestinationHeight, 8.0f), "32 authored pixels at quarter scale is 8 vertically");
    Check(NearlyEqual(geometry.Radius, 0.5f), "a radius of 2 at quarter scale is half a pixel");
    Check(geometry.Radius > 0.0f, "the sub-pixel radius is kept rather than flattened to zero");
    Check(Nintendo64RoundedRectGeometry::RoundToInt(geometry.DestinationWidth) == 8,
          "the traced swatch size rounds to 8");
}

/// A rectangle with no area cannot be filled, and the caller needs to hear that rather than guess.
void TestNonPositiveSizeReportsEmptyDestination() {
    const Nintendo64RoundedRectFillGeometry zeroWidth =
        Nintendo64RoundedRectGeometry::Resolve(0, 32, 4.0f, 1.0f, 1.0f);
    Check(zeroWidth.Kind == Nintendo64RoundedRectGeometryKind::EmptyDestination,
          "a zero authored width reports an empty destination");
    Check(NearlyEqual(zeroWidth.DestinationWidth, 0.0f), "the empty destination width is reported as zero");
    Check(NearlyEqual(zeroWidth.Radius, 0.0f), "an empty destination leaves the radius unset");

    const Nintendo64RoundedRectFillGeometry negativeHeight =
        Nintendo64RoundedRectGeometry::Resolve(32, -4, 4.0f, 1.0f, 1.0f);
    Check(negativeHeight.Kind == Nintendo64RoundedRectGeometryKind::EmptyDestination,
          "a negative authored height reports an empty destination");
    Check(NearlyEqual(negativeHeight.DestinationHeight, -4.0f),
          "the collapsed height is still reported so the skip trace can name it");

    const Nintendo64RoundedRectFillGeometry zeroScale =
        Nintendo64RoundedRectGeometry::Resolve(32, 32, 4.0f, 0.0f, 1.0f);
    Check(zeroScale.Kind == Nintendo64RoundedRectGeometryKind::EmptyDestination,
          "a zero parent scale reports an empty destination");

    const Nintendo64RoundedRectFillGeometry negativeScale =
        Nintendo64RoundedRectGeometry::Resolve(32, 32, 4.0f, 1.0f, -0.5f);
    Check(negativeScale.Kind == Nintendo64RoundedRectGeometryKind::EmptyDestination,
          "a negative parent scale reports an empty destination");
}

void TestRoundToIntRoundsHalvesAway() {
    Check(Nintendo64RoundedRectGeometry::RoundToInt(7.5f) == 8, "7.5 rounds up");
    Check(Nintendo64RoundedRectGeometry::RoundToInt(7.4f) == 7, "7.4 rounds down");
    Check(Nintendo64RoundedRectGeometry::RoundToInt(-7.5f) == -8, "-7.5 rounds away from zero");
}

} // namespace

int main() {
    TestUnitScaleKeepsTheAuthoredNumbers();
    TestParentScaleAppliesToBothAxesAndTheRadius();
    TestRadiusClampsToHalfTheSmallerSide();
    TestSubPixelRadiusStaysDrawable();
    TestZeroRadiusSelectsTheDirectRectanglePath();
    TestNonPositiveSizeReportsEmptyDestination();
    TestRoundToIntRoundsHalvesAway();
    if (failures != 0) {
        std::fprintf(stderr, "%d rounded-rect-geometry test(s) failed\n", failures);
        return 1;
    }
    std::puts("n64-rounded-rect-geometry-tests: PASS");
    return 0;
}
