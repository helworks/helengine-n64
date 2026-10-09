#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <libdragon.h>

#include "RuntimeTexture.hpp"
#include "TextureAsset.hpp"

namespace helengine::n64 {

/// Owns uncached RGBA5551 or CI4/CI8 texels and an optional independent RDP TLUT.
class Nintendo64RuntimeTexture final : public ::RuntimeTexture {
public:
    /// Creates an empty texture that owns no surface until `Initialize` succeeds.
    Nintendo64RuntimeTexture();
    /// Releases the surface through `Dispose`.
    ~Nintendo64RuntimeTexture() override;

    /// Allocates a direct RGBA5551 surface or indexed CI surface/TLUT from a validated cooked payload.
    /// Any previously owned surface is released first, so a second call replaces the texture rather than leaking it;
    /// an unusable asset is rejected before that release, leaving an already initialized texture untouched, while a
    /// failed allocation leaves the texture with no surface and `Surface()` null.
    bool Initialize(const ::TextureAsset* asset);
    /// Surface consumed by the RDP blitter, or null before initialization.
    const surface_t* Surface() const { return SurfaceInitialized ? &SurfaceValue : nullptr; }
    /// Selects the TLUT mode; uploads bank zero unless the current executor has already loaded this immutable palette.
    /// External callers default to a complete upload because they cannot assume the previous TMEM contents.
    void Bind(bool uploadPalette = true) const;
    /// True when this texture samples an owned TLUT rather than direct texels.
    bool IsIndexed() const { return PaletteEntryCount != 0; }
    /// Owned native texel bytes, excluding the separately owned palette.
    size_t TexelBytes() const { return SurfaceInitialized ? static_cast<size_t>(SurfaceValue.stride) * Height : 0; }
    /// Native palette storage; CI4 reserves sixteen entries and CI8 reserves 256.
    size_t PaletteBytes() const { return PaletteEntryCount * sizeof(uint16_t); }
    /// Releases the surface and marks the texture disposed; safe to call more than once.
    void Dispose() override;

private:
    /// Frees any owned surface, clears the initialized flag and the reported dimensions, and leaves the
    /// disposal state alone; does nothing when no surface is owned, so it is safe to call at any time.
    /// This is the one place that frees the surface: both `Dispose` and `Initialize` go through it.
    void ReleaseSurface();

    /// libdragon surface holding converted RGBA5551 texels or unchanged/repacked indices.
    surface_t SurfaceValue{};
    /// True once `SurfaceValue` owns an allocated buffer that `ReleaseSurface` must free.
    bool SurfaceInitialized = false;
    /// Uncached, aligned TLUT source retained until all queued RDP uploads have completed.
    uint16_t* PaletteValue = nullptr;
    /// Hardware upload count, including zero-filled unreferenced palette slots.
    int PaletteEntryCount = 0;
    /// Palette interpretation; direct textures select TLUT_NONE to clear prior indexed state.
    rdpq_tlut_t PaletteMode = TLUT_NONE;
};

} // namespace helengine::n64

#endif
