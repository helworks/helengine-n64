#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "src/platform/n64/Nintendo64ClipRectStack.hpp"

namespace {

using helengine::n64::Nintendo64ClipRect;
using helengine::n64::Nintendo64ClipRectStack;

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

/// With nothing pushed the clip is the whole frame, which is what the RDP scissor starts at.
void TestEmptyStackIsTheWholeFrame() {
    const Nintendo64ClipRectStack stack(320, 240);
    const Nintendo64ClipRect current = stack.Current();

    Check(stack.Depth() == 0, "a fresh stack has no pushed rectangles");
    Check(!current.IsEmpty, "the whole frame is not empty");
    Check(current.X == 0 && current.Y == 0, "the whole frame starts at the origin");
    Check(current.Width == 320 && current.Height == 240, "the whole frame is the frame size");
}

/// A pushed rectangle inside the frame is used as authored, rounded to whole pixels.
void TestPushedRectIsUsedAsAuthored() {
    Nintendo64ClipRectStack stack(320, 240);
    stack.Push(10.0f, 20.0f, 100.0f, 50.0f);
    const Nintendo64ClipRect current = stack.Current();

    Check(stack.Depth() == 1, "one push is one level deep");
    Check(current.X == 10 && current.Y == 20, "the pushed origin survives");
    Check(current.Width == 100 && current.Height == 50, "the pushed extent survives");
}

/// A pushed rectangle is clamped to the frame, because the RDP scissor cannot address pixels the frame
/// buffer does not have.
void TestPushedRectIsClampedToTheFrame() {
    Nintendo64ClipRectStack stack(320, 240);
    stack.Push(-40.0f, 200.0f, 400.0f, 400.0f);
    const Nintendo64ClipRect current = stack.Current();

    Check(current.X == 0, "a rectangle starting off the left edge begins at zero");
    Check(current.Y == 200, "a rectangle inside the frame vertically keeps its top");
    Check(current.Width == 320, "the width is clamped to the frame width");
    Check(current.Height == 40, "the height is clamped to what remains below the top");
}

/// A nested rectangle intersects its parent. An inner rectangle larger than its parent must not widen it,
/// which is the whole point of nesting.
void TestNestedRectIntersectsItsParent() {
    Nintendo64ClipRectStack stack(320, 240);
    stack.Push(100.0f, 100.0f, 50.0f, 50.0f);
    stack.Push(0.0f, 0.0f, 320.0f, 240.0f);
    const Nintendo64ClipRect current = stack.Current();

    Check(stack.Depth() == 2, "two pushes are two levels deep");
    Check(current.X == 100 && current.Y == 100, "the inner rectangle cannot move outside its parent");
    Check(current.Width == 50 && current.Height == 50, "the inner rectangle cannot grow past its parent");
}

/// Two disjoint rectangles intersect in nothing, and the result must say so rather than report a negative
/// extent the scissor would reinterpret.
void TestDisjointNestedRectIsEmpty() {
    Nintendo64ClipRectStack stack(320, 240);
    stack.Push(0.0f, 0.0f, 100.0f, 100.0f);
    stack.Push(200.0f, 200.0f, 50.0f, 50.0f);
    const Nintendo64ClipRect current = stack.Current();

    Check(current.IsEmpty, "disjoint rectangles intersect in nothing");
    Check(current.Width == 0 && current.Height == 0, "an empty rectangle has no extent");
}

/// Once a level is empty, everything nested inside it stays empty.
void TestEmptyParentKeepsChildrenEmpty() {
    Nintendo64ClipRectStack stack(320, 240);
    stack.Push(0.0f, 0.0f, 100.0f, 100.0f);
    stack.Push(200.0f, 200.0f, 50.0f, 50.0f);
    stack.Push(0.0f, 0.0f, 320.0f, 240.0f);

    Check(stack.Current().IsEmpty, "a child of an empty clip is empty");
}

/// Popping restores the enclosing rectangle rather than the whole frame.
void TestPopRestoresTheEnclosingRect() {
    Nintendo64ClipRectStack stack(320, 240);
    stack.Push(100.0f, 100.0f, 50.0f, 50.0f);
    stack.Push(110.0f, 110.0f, 10.0f, 10.0f);
    stack.Pop();
    const Nintendo64ClipRect current = stack.Current();

    Check(stack.Depth() == 1, "a pop removes one level");
    Check(current.X == 100 && current.Width == 50, "the enclosing rectangle comes back, not the frame");
}

/// Popping an empty stack is a programming error in the executor, not a recoverable state.
void TestPopOnAnEmptyStackThrows() {
    Nintendo64ClipRectStack stack(320, 240);
    bool threw = false;
    try {
        stack.Pop();
    } catch (const std::underflow_error&) {
        threw = true;
    }

    Check(threw, "popping an empty clip stack throws");
}

/// Pushing past the fixed depth is a programming error too. The stack is fixed so it allocates nothing.
void TestPushPastTheDepthLimitThrows() {
    Nintendo64ClipRectStack stack(320, 240);
    bool threw = false;
    try {
        for (int32_t index = 0; index < 64; ++index) {
            stack.Push(0.0f, 0.0f, 320.0f, 240.0f);
        }
    } catch (const std::length_error&) {
        threw = true;
    }

    Check(threw, "pushing past the depth limit throws");
}

/// Asymmetric frame and nested push with distinct X and Y values catch X/Y axis confusion.
void TestAxisCoverageWithAsymmetricFrame() {
    Nintendo64ClipRectStack stack(320, 240);
    stack.Push(60.0f, 100.0f, 100.0f, 50.0f);
    stack.Push(-10.0f, -10.0f, 320.0f, 240.0f);
    const Nintendo64ClipRect current = stack.Current();

    Check(current.X == 60, "a nested rectangle starting outside the parent clips to the parent's left");
    Check(current.Y == 100, "a nested rectangle starting outside the parent clips to the parent's top (not its left)");
    Check(current.Width == 100, "the width is clamped correctly");
    Check(current.Height == 50, "the height is clamped correctly");
}

/// Constructing a stack with a non-positive frame width is a programming error.
void TestConstructorThrowsOnNonPositiveWidth() {
    bool threw = false;
    try {
        Nintendo64ClipRectStack stack(0, 240);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    Check(threw, "constructing with zero width throws");
}

/// Constructing a stack with a non-positive frame height is a programming error.
void TestConstructorThrowsOnNonPositiveHeight() {
    bool threw = false;
    try {
        Nintendo64ClipRectStack stack(320, -1);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    Check(threw, "constructing with negative height throws");
}

} // namespace

int main() {
    TestEmptyStackIsTheWholeFrame();
    TestPushedRectIsUsedAsAuthored();
    TestPushedRectIsClampedToTheFrame();
    TestNestedRectIntersectsItsParent();
    TestDisjointNestedRectIsEmpty();
    TestEmptyParentKeepsChildrenEmpty();
    TestPopRestoresTheEnclosingRect();
    TestPopOnAnEmptyStackThrows();
    TestPushPastTheDepthLimitThrows();
    TestAxisCoverageWithAsymmetricFrame();
    TestConstructorThrowsOnNonPositiveWidth();
    TestConstructorThrowsOnNonPositiveHeight();
    if (failures != 0) {
        std::fprintf(stderr, "%d clip-rect-stack test(s) failed\n", failures);
        return 1;
    }
    std::puts("n64-clip-rect-stack-tests: PASS");
    return 0;
}
