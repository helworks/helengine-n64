#include "platform/n64/Nintendo64RuntimeTexture.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cstddef>
#include <cstdint>

#include "runtime/array.hpp"

#include "platform/n64/Nintendo64TexelPacking.hpp"

namespace helengine::n64 {

Nintendo64RuntimeTexture::Nintendo64RuntimeTexture() = default;

Nintendo64RuntimeTexture::~Nintendo64RuntimeTexture() {
    Dispose();
}

bool Nintendo64RuntimeTexture::Initialize(const ::TextureAsset* asset) {
    if (asset == nullptr || asset->Colors == nullptr) {
        return false;
    }
    if (asset->ColorFormat != ::TextureAssetColorFormat::Rgba32) {
        return false; // the N64 cooker emits straight RGBA32; any other format is a cooker defect
    }
    const int32_t width = static_cast<int32_t>(asset->Width);
    const int32_t height = static_cast<int32_t>(asset->Height);
    if (width <= 0 || height <= 0) {
        return false;
    }
    const int64_t expectedLength = static_cast<int64_t>(width) * static_cast<int64_t>(height) * 4;
    if (static_cast<int64_t>(asset->Colors->get_Length()) != expectedLength) {
        return false; // a truncated payload is a cooker defect, not something to pad around
    }

    // Release any surface a previous call left behind only once the asset is known good, so a
    // rejected asset cannot destroy a texture that is already usable.  This deliberately does not
    // go through Dispose, which would mark a texture we are about to bring to life as disposed.
    ReleaseSurface();

    SurfaceValue = surface_alloc(FMT_RGBA16, static_cast<uint16_t>(width), static_cast<uint16_t>(height));
    if (SurfaceValue.buffer == nullptr) {
        SurfaceValue = surface_t{}; // leave no half-populated surface behind for a failed allocation
        return false;
    }
    for (int32_t rowIndex = 0; rowIndex < height; rowIndex++) {
        const uint8_t* sourceRow = &(*asset->Colors)[rowIndex * width * 4];
        uint16_t* destinationRow = reinterpret_cast<uint16_t*>(
            static_cast<uint8_t*>(SurfaceValue.buffer) + static_cast<size_t>(rowIndex) * SurfaceValue.stride);
        Nintendo64TexelPacking::ConvertRow(sourceRow, width, destinationRow);
    }
    SurfaceInitialized = true;
    Width = width;
    Height = height;
    IsEngineOwned = false;
    return true;
}

void Nintendo64RuntimeTexture::ReleaseSurface() {
    if (SurfaceInitialized) {
        surface_free(&SurfaceValue);
        SurfaceValue = surface_t{};
        SurfaceInitialized = false;
    }
    Width = 0;
    Height = 0;
}

void Nintendo64RuntimeTexture::Dispose() {
    ReleaseSurface();
    ::RuntimeTexture::Dispose();
}

} // namespace helengine::n64

#endif
