#include <cmath>
#include <cstdint>
#include <cstdio>

#include "src/platform/n64/FrameClock.hpp"

namespace {

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

void CheckNear(float actual, float expected, float tolerance, const char* message) {
    Check(std::fabs(actual - expected) <= tolerance, message);
}

void TestFirstFrameAndBoundedDelta() {
    helengine::n64::FrameClock clock;
    clock.Reset(1'000'000);

    Check(clock.Advance(1'016'000) == 16'000, "normal delta is measured in microseconds");
    CheckNear(clock.AngleX(), 16'000.0f * 0.35e-6f, 1e-6f,
              "X angle integrates normal elapsed time");
    CheckNear(clock.AngleY(), 16'000.0f * 0.55e-6f, 1e-6f,
              "Y angle integrates normal elapsed time");

    Check(clock.Advance(2'000'000) == 100'000, "large delta is capped at 100 ms");
    CheckNear(clock.AngleX(), (16'000.0f + 100'000.0f) * 0.35e-6f, 1e-6f,
              "X angle integrates capped elapsed time");
}

void TestBackwardTimestampDoesNotCreateHugeStep() {
    helengine::n64::FrameClock clock;
    clock.Reset(5'000'000);

    Check(clock.Advance(4'000'000) == 0, "backward timestamp produces zero delta");
    Check(clock.Advance(4'016'000) == 16'000,
          "clock resumes from the backward timestamp without a huge jump");
}

void TestAnglesWrapToOneTurn() {
    helengine::n64::FrameClock clock;
    clock.Reset(0);
    clock.Advance(100'000'000);

    Check(clock.AngleX() >= 0.0f && clock.AngleX() < 6.2831855f,
          "X angle remains within one turn");
    Check(clock.AngleY() >= 0.0f && clock.AngleY() < 6.2831855f,
          "Y angle remains within one turn");
}

} // namespace

int main() {
    TestFirstFrameAndBoundedDelta();
    TestBackwardTimestampDoesNotCreateHugeStep();
    TestAnglesWrapToOneTurn();
    if (failures != 0) {
        std::fprintf(stderr, "%d frame-clock test(s) failed\n", failures);
        return 1;
    }
    std::puts("frame-clock-tests: PASS");
    return 0;
}
