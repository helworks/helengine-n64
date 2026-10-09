#include "platform/n64/Nintendo64RuntimeTexture.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cstddef>
#include <cstdint>

#include "runtime/array.hpp"

#include "platform/n64/Nintendo64TexelPacking.hpp"
#include "platform/n64/Nintendo64IndexedTexturePacking.hpp"

namespace helengine::n64 {

Nintendo64RuntimeTexture::Nintendo64RuntimeTexture() = default;

Nintendo64RuntimeTexture::~Nintendo64RuntimeTexture() {
    Dispose();
}

bool Nintendo64RuntimeTexture::Initialize(const ::TextureAsset* asset) {
    if (asset == nullptr || asset->Colors == nullptr) {
        return false;
    }
    const bool fourBit = asset->ColorFormat == ::TextureAssetColorFormat::Indexed4;
    const bool indexed = fourBit || asset->ColorFormat == ::TextureAssetColorFormat::Indexed8;
    if (!indexed && asset->ColorFormat != ::TextureAssetColorFormat::Rgba32) return false;
    const int32_t width = static_cast<int32_t>(asset->Width);
    const int32_t height = static_cast<int32_t>(asset->Height);
    if (width <= 0 || height <= 0 || width > 32767 || height > 65535) {
        return false;
    }
    const int64_t pixels = static_cast<int64_t>(width) * height;
    const int64_t expectedLength = indexed ? (fourBit ? (pixels + 1) / 2 : pixels) : pixels * 4;
    if (static_cast<int64_t>(asset->Colors->get_Length()) != expectedLength) {
        return false; // a truncated payload is a cooker defect, not something to pad around
    }
    if (indexed && (asset->PaletteColors == nullptr ||
        !Nintendo64IndexedTexturePacking::Validate(&(*asset->Colors)[0], static_cast<size_t>(expectedLength),
            static_cast<size_t>(pixels), fourBit, asset->PaletteColors->get_Length() ? &(*asset->PaletteColors)[0] : nullptr,
            static_cast<size_t>(asset->PaletteColors->get_Length()), static_cast<uint8_t>(asset->AlphaPrecision)))) return false;

    // Release any surface a previous call left behind only once the asset is known good, so a
    // rejected asset cannot destroy a texture that is already usable.  This deliberately does not
    // go through Dispose, which would mark a texture we are about to bring to life as disposed.
    ReleaseSurface();

    SurfaceValue = surface_alloc(indexed ? (fourBit ? FMT_CI4 : FMT_CI8) : FMT_RGBA16,
        static_cast<uint16_t>(width), static_cast<uint16_t>(height));
    if (SurfaceValue.buffer == nullptr) {
        SurfaceValue = surface_t{}; // leave no half-populated surface behind for a failed allocation
        return false;
    }
    if (indexed) {
        PaletteEntryCount = fourBit ? 16 : 256;
        PaletteValue = static_cast<uint16_t*>(malloc_uncached_aligned(64, PaletteBytes()));
        if (PaletteValue == nullptr) {
            surface_free(&SurfaceValue);
            SurfaceValue = surface_t{};
            PaletteEntryCount = 0;
            return false;
        }
        const size_t entries = static_cast<size_t>(asset->PaletteColors->get_Length()) / 4;
        const uint8_t* palette = &(*asset->PaletteColors)[0];
        const bool grayscale = Nintendo64IndexedTexturePacking::IsGrayscale(palette, entries);
        PaletteMode = grayscale ? TLUT_IA16 : TLUT_RGBA16;
        Nintendo64IndexedTexturePacking::ConvertPalette(palette, entries, static_cast<uint8_t>(asset->AlphaPrecision),
            grayscale, PaletteValue, static_cast<size_t>(PaletteEntryCount));
        Nintendo64IndexedTexturePacking::CopyRows(&(*asset->Colors)[0], width, height, fourBit,
            static_cast<uint8_t*>(SurfaceValue.buffer), SurfaceValue.stride);
    } else for (int32_t rowIndex = 0; rowIndex < height; rowIndex++) {
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
        // libdragon documents rspq_wait as waiting for queued RSP commands AND their RDP draws.
        // TLUT uploads retain the uncached source pointer, so both allocations must outlive that fence.
        rspq_wait();
        surface_free(&SurfaceValue);
        SurfaceValue = surface_t{};
        SurfaceInitialized = false;
    }
    if (PaletteValue != nullptr) free_uncached(PaletteValue);
    PaletteValue = nullptr;
    PaletteEntryCount = 0;
    PaletteMode = TLUT_NONE;
    Width = 0;
    Height = 0;
}

void Nintendo64RuntimeTexture::Bind(bool uploadPalette) const {
    if (!SurfaceInitialized) return;
    rdpq_mode_tlut(PaletteMode);
    if (PaletteValue != nullptr && uploadPalette) rdpq_tex_upload_tlut(PaletteValue, 0, PaletteEntryCount);
}

void Nintendo64RuntimeTexture::Dispose() {
    ReleaseSurface();
    ::RuntimeTexture::Dispose();
}

} // namespace helengine::n64

#endif
