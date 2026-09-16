#include <cstdint>
#include <cstdio>

#include "src/platform/n64/Nintendo64TexelPacking.hpp"

namespace {

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

void TestPacksChannelsIntoFiveFiveFiveOne() {
    Check(helengine::n64::Nintendo64TexelPacking::PackRgba5551(255, 255, 255, 255) == 0xFFFFu,
          "opaque white packs to all bits set");
    Check(helengine::n64::Nintendo64TexelPacking::PackRgba5551(0, 0, 0, 255) == 0x0001u,
          "opaque black keeps only the alpha bit");
    Check(helengine::n64::Nintendo64TexelPacking::PackRgba5551(255, 0, 0, 255) == 0xF801u,
          "red occupies the top five bits");
    Check(helengine::n64::Nintendo64TexelPacking::PackRgba5551(0, 255, 0, 255) == 0x07C1u,
          "green occupies the middle five bits");
    Check(helengine::n64::Nintendo64TexelPacking::PackRgba5551(0, 0, 255, 255) == 0x003Fu,
          "blue occupies the low five bits plus alpha");
}

void TestAlphaThresholdIsHalf() {
    Check((helengine::n64::Nintendo64TexelPacking::PackRgba5551(255, 255, 255, 127) & 1u) == 0u,
          "alpha below the threshold clears the alpha bit");
    Check((helengine::n64::Nintendo64TexelPacking::PackRgba5551(255, 255, 255, 128) & 1u) == 1u,
          "alpha at the threshold sets the alpha bit");
}

void TestConvertRowWritesEveryTexel() {
    const uint8_t rgba[8] = {255, 0, 0, 255, 0, 0, 255, 0};
    uint16_t output[2] = {0x1234u, 0x5678u};
    helengine::n64::Nintendo64TexelPacking::ConvertRow(rgba, 2, output);
    Check(output[0] == 0xF801u, "first texel converts");
    Check(output[1] == 0x003Eu, "second texel converts with a clear alpha bit");
}

} // namespace

int main() {
    TestPacksChannelsIntoFiveFiveFiveOne();
    TestAlphaThresholdIsHalf();
    TestConvertRowWritesEveryTexel();
    if (failures != 0) {
        std::fprintf(stderr, "%d texel-packing test(s) failed\n", failures);
        return 1;
    }
    std::puts("n64-texel-packing-tests: PASS");
    return 0;
}
