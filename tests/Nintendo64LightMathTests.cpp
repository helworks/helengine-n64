#include <cmath>
#include <cstdint>
#include <cstdio>

#include "src/platform/n64/Nintendo64LightMath.hpp"

namespace {

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

void TestPackColorScalesAndClamps() {
    uint8_t color[4];
    helengine::n64::Nintendo64LightMath::PackColor(1.0f, 0.5f, 0.0f, 1.0f, color);
    Check(color[0] == 255 && color[1] == 128 && color[2] == 0 && color[3] == 255, "unit intensity maps 1/0.5/0 to 255/128/0");
    helengine::n64::Nintendo64LightMath::PackColor(1.0f, 1.0f, 1.0f, 2.0f, color);
    Check(color[0] == 255 && color[1] == 255 && color[2] == 255, "intensity above one clamps to 255");
    helengine::n64::Nintendo64LightMath::PackColor(0.5f, 0.5f, 0.5f, 0.5f, color);
    Check(color[0] == 64 && color[1] == 64 && color[2] == 64, "intensity scales before packing");
}

void TestAccumulatedAmbientSumsThenClamps() {
    float accumulator[3] = {0.0f, 0.0f, 0.0f};
    helengine::n64::Nintendo64LightMath::Accumulate(0.25f, 0.25f, 0.25f, 1.0f, accumulator);
    helengine::n64::Nintendo64LightMath::Accumulate(0.25f, 1.0f, 0.25f, 1.0f, accumulator);
    uint8_t color[4];
    helengine::n64::Nintendo64LightMath::PackAccumulated(accumulator, color);
    Check(color[0] == 128 && color[1] == 255 && color[2] == 128 && color[3] == 255, "two ambient lights sum and clamp");
}

void TestLightVectorIsNegatedAndNormalized() {
    float output[3];
    Check(helengine::n64::Nintendo64LightMath::ToLightVector(0.0f, -2.0f, 0.0f, output), "non-zero forward converts");
    Check(std::fabs(output[0]) < 1e-6f && std::fabs(output[1] - 1.0f) < 1e-6f && std::fabs(output[2]) < 1e-6f,
          "a light pointing down yields a surface-to-light vector pointing up");
    Check(!helengine::n64::Nintendo64LightMath::ToLightVector(0.0f, 0.0f, 0.0f, output), "zero forward is rejected");
}

} // namespace

int main() {
    TestPackColorScalesAndClamps();
    TestAccumulatedAmbientSumsThenClamps();
    TestLightVectorIsNegatedAndNormalized();
    if (failures != 0) {
        std::fprintf(stderr, "%d light-math test(s) failed\n", failures);
        return 1;
    }
    std::puts("n64-light-math-tests: PASS");
    return 0;
}
