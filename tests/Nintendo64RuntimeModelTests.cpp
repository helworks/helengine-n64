#include <cstdio>
#include <cstring>
#include <limits>
#include "platform/n64/Nintendo64RuntimeModel.hpp"
#include "ModelAsset.hpp"

namespace {
int failures = 0;
void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}
ModelAsset* Triangle(int uvCount = 3) {
    auto* asset = new ModelAsset();
    asset->Positions = new Array<float3>(3);
    (*asset->Positions)[0] = {0, 0, 0};
    (*asset->Positions)[1] = {1, 0, 0};
    (*asset->Positions)[2] = {0, 1, 0};
    asset->Indices16 = new Array<uint16_t>(3);
    for (int32_t index = 0; index < 3; ++index) (*asset->Indices16)[index] = static_cast<uint16_t>(index);
    if (uvCount >= 0) {
        asset->TexCoords = new Array<float2>(uvCount);
        for (int32_t index = 0; index < uvCount; ++index) (*asset->TexCoords)[index] = {0.25f, 0.75f};
    }
    return asset;
}
void TestSharedModelUsesOriginalUvsForEveryTextureSize() {
    helengine::n64::Nintendo64RuntimeModel model;
    ModelAsset* asset = Triangle();
    Check(model.Initialize(asset), "textured triangle initializes the production runtime model");
    Check(model.HasTextureCoordinates(), "complete UV array is reported");
    Check(model.get_RawModelAsset() == asset, "original UV storage is retained by RuntimeModel");
    model.SetColor(0x11223344u);
    Check(model.SetTextureDimensions(64, 32), "first texture dimensions are accepted");
    Check(model.Vertices()[0].stA[0] == 512 && model.Vertices()[0].stA[1] == 768,
          "first material scales UVs to 64 by 32 texels");
    Check(model.Vertices()[0].stB[0] == 512 && model.Vertices()[1].stA[1] == 768,
          "every logical vertex including odd final pair is converted");
    Check(model.SetTextureDimensions(16, 8), "shared geometry accepts a different material texture size");
    Check(model.Vertices()[0].stA[0] == 128 && model.Vertices()[0].stA[1] == 192,
          "second material scales original UVs instead of already packed UVs");
    Check(model.SetTextureDimensions(64, 32), "texture size can switch back on a later draw");
    Check(model.Vertices()[0].stA[0] == 512 && model.Vertices()[0].stA[1] == 768,
          "switching back restores original packed coordinates");
    Check(model.Vertices()[0].rgbaA == 0x11223344u && model.Vertices()[0].posB[0] == 64,
          "texture changes preserve colors and geometry");
    Check((*asset->TexCoords)[0].X == 0.25f && (*asset->TexCoords)[0].Y == 0.75f,
          "source normalized UVs are never mutated");
    model.Dispose();
    Check(!model.HasTextureCoordinates() && !model.SetTextureDimensions(64, 32),
          "disposed model loses texture-coordinate capability");
}
void TestInvalidUvsAndMissingUvs() {
    for (int count : {-1, 0, 2, 4}) {
        auto* asset = Triangle(count);
        helengine::n64::Nintendo64RuntimeModel model;
        const bool initialized = model.Initialize(asset);
        if (count == 2 || count == 4) {
            Check(!initialized, "partial or excess UV array is rejected as incompatible");
            if (!initialized) delete asset;
        } else {
            Check(initialized && !model.HasTextureCoordinates(), "untextured geometry remains valid");
            Check(!model.SetTextureDimensions(16, 16), "untextured geometry rejects texture-coordinate updates");
        }
    }
    auto* asset = Triangle();
    (*asset->TexCoords)[2].Y = std::numeric_limits<float>::infinity();
    helengine::n64::Nintendo64RuntimeModel model;
    const bool initialized = model.Initialize(asset);
    Check(!initialized, "nonfinite UV is rejected during model initialization");
    if (!initialized) delete asset;
}
void TestSameDimensionsObserveMutableRetainedUvs() {
    auto* asset = Triangle();
    helengine::n64::Nintendo64RuntimeModel model;
    Check(model.Initialize(asset) && model.SetTextureDimensions(16, 8), "mutable raw-UV fixture initializes");
    (*asset->TexCoords)[0] = {0.5f, 0.25f};
    Check(model.SetTextureDimensions(16, 8), "same dimensions still observe retained raw UV edits");
    Check(model.Vertices()[0].stA[0] == 256 && model.Vertices()[0].stA[1] == 64,
          "no dimension-only cache can hide a publicly mutable RawModelAsset");
    const T3DVertPacked before = model.Vertices()[0];
    (*asset->TexCoords)[2].X = std::numeric_limits<float>::quiet_NaN();
    Check(!model.SetTextureDimensions(16, 8), "same dimensions revalidate a later nonfinite raw UV");
    Check(std::memcmp(&before, model.Vertices(), sizeof(before)) == 0,
          "invalid retained UV edit cannot partially update previous vertices");
}
void TestRejectedDimensionsCannotPartiallyRewriteGeometry() {
    auto* asset = Triangle();
    (*asset->TexCoords)[2] = {2.0f, -1.0f};
    helengine::n64::Nintendo64RuntimeModel model;
    Check(model.Initialize(asset) && model.SetTextureDimensions(32, 32), "wrapped UVs fit a small texture");
    T3DVertPacked before[2];
    std::memcpy(before, model.Vertices(), sizeof(before));
    Check(!model.SetTextureDimensions(1024, 32), "a later vertex overflowing signed16 rejects the texture size");
    Check(std::memcmp(before, model.Vertices(), sizeof(before)) == 0,
          "range failure leaves earlier vertices and every other field unchanged");
    Check(!model.SetTextureDimensions(32, 0), "zero texture dimension is rejected");
    Check(std::memcmp(before, model.Vertices(), sizeof(before)) == 0, "dimension failure preserves packed data");
    Check(model.SetTextureDimensions(16, 8), "valid update succeeds after a rejected texture size");
    Check(model.Vertices()[1].stA[0] == 1024 && model.Vertices()[1].stA[1] == -256,
          "negative/repeated coordinates preserve the SDK UV convention");
}
}
int main() {
    TestSharedModelUsesOriginalUvsForEveryTextureSize();
    TestInvalidUvsAndMissingUvs();
    TestRejectedDimensionsCannotPartiallyRewriteGeometry();
    TestSameDimensionsObserveMutableRetainedUvs();
    if (failures != 0) return 1;
    std::puts("n64-runtime-model-tests: PASS");
    return 0;
}
