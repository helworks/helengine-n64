#include <cstdint>
#include <cstdio>

#include "src/platform/n64/Nintendo64VertexPacking.hpp"

namespace {

int failures = 0;

/// Records a failure with its message when the asserted condition does not hold.
void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

/// Fills every field with a distinct sentinel so an untouched slot is provably untouched.
T3DVertPacked MakeSentinelVertex() {
    T3DVertPacked packed = {};
    packed.posA[0] = 0x1111;
    packed.posA[1] = 0x1112;
    packed.posA[2] = 0x1113;
    packed.normA = 0xAAAA;
    packed.posB[0] = 0x2221;
    packed.posB[1] = 0x2222;
    packed.posB[2] = 0x2223;
    packed.normB = 0xBBBB;
    packed.rgbaA = 0xCCCCCCCCu;
    packed.rgbaB = 0xDDDDDDDDu;
    packed.stA[0] = 0x3331;
    packed.stA[1] = 0x3332;
    packed.stB[0] = 0x4441;
    packed.stB[1] = 0x4442;
    return packed;
}

void TestEvenIndexWritesSlotAOnly() {
    T3DVertPacked packed = MakeSentinelVertex();
    const float position[3] = {1.5f, -0.26f, 0.0f};
    helengine::n64::Nintendo64VertexPacking::WriteVertex(packed, 0, position, 64.0f, 0x1234, 0xFF00FF00u);

    Check(packed.posA[0] == 96, "1.5 scaled by 64 rounds to 96 in slot A");
    Check(packed.posA[1] == -17, "-0.26 scaled by 64 is -16.64 and rounds away from zero to -17");
    Check(packed.posA[2] == 0, "zero scales to zero in slot A");
    Check(packed.normA == 0x1234, "the packed normal lands in slot A");
    Check(packed.rgbaA == 0xFF00FF00u, "the color lands in slot A");
    Check(packed.stA[0] == 0 && packed.stA[1] == 0, "slot A UVs are zeroed");

    Check(packed.posB[0] == 0x2221 && packed.posB[1] == 0x2222 && packed.posB[2] == 0x2223,
          "an even index leaves the slot B position untouched");
    Check(packed.normB == 0xBBBB, "an even index leaves the slot B normal untouched");
    Check(packed.rgbaB == 0xDDDDDDDDu, "an even index leaves the slot B color untouched");
    Check(packed.stB[0] == 0x4441 && packed.stB[1] == 0x4442, "an even index leaves the slot B UVs untouched");
}

void TestOddIndexWritesSlotBOnly() {
    T3DVertPacked packed = MakeSentinelVertex();
    const float position[3] = {1.5f, -0.26f, -2.0f};
    helengine::n64::Nintendo64VertexPacking::WriteVertex(packed, 1, position, 64.0f, 0x5678, 0x00FF00FFu);

    Check(packed.posB[0] == 96, "1.5 scaled by 64 rounds to 96 in slot B");
    Check(packed.posB[1] == -17, "-0.26 scaled by 64 is -16.64 and rounds away from zero to -17");
    Check(packed.posB[2] == -128, "-2.0 scaled by 64 is -128 in slot B");
    Check(packed.normB == 0x5678, "the packed normal lands in slot B");
    Check(packed.rgbaB == 0x00FF00FFu, "the color lands in slot B");
    Check(packed.stB[0] == 0 && packed.stB[1] == 0, "slot B UVs are zeroed");

    Check(packed.posA[0] == 0x1111 && packed.posA[1] == 0x1112 && packed.posA[2] == 0x1113,
          "an odd index leaves the slot A position untouched");
    Check(packed.normA == 0xAAAA, "an odd index leaves the slot A normal untouched");
    Check(packed.rgbaA == 0xCCCCCCCCu, "an odd index leaves the slot A color untouched");
    Check(packed.stA[0] == 0x3331 && packed.stA[1] == 0x3332, "an odd index leaves the slot A UVs untouched");
}

void TestHigherIndicesAlternateSlotsAndHonourTheScale() {
    T3DVertPacked packed = MakeSentinelVertex();
    const float position[3] = {1.0f, 2.0f, 3.0f};
    helengine::n64::Nintendo64VertexPacking::WriteVertex(packed, 2, position, 1.0f, 0, 0u);
    Check(packed.posA[0] == 1 && packed.posA[1] == 2 && packed.posA[2] == 3,
          "logical index 2 writes slot A and a unit scale leaves positions unscaled");
    Check(packed.normA == 0 && packed.rgbaA == 0u, "a zero normal and color are stored as given");

    helengine::n64::Nintendo64VertexPacking::WriteVertex(packed, 3, position, 100.0f, 1, 1u);
    Check(packed.posB[0] == 100 && packed.posB[1] == 200 && packed.posB[2] == 300,
          "logical index 3 writes slot B and positions scale by the supplied scale");
}

} // namespace

int main() {
    TestEvenIndexWritesSlotAOnly();
    TestOddIndexWritesSlotBOnly();
    TestHigherIndicesAlternateSlotsAndHonourTheScale();
    if (failures != 0) {
        std::fprintf(stderr, "%d vertex-packing test(s) failed\n", failures);
        return 1;
    }
    std::puts("n64-vertex-packing-tests: PASS");
    return 0;
}
