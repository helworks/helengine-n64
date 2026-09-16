#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <libdragon.h>

#include "RuntimeTexture.hpp"
#include "TextureAsset.hpp"

namespace helengine::n64 {

/// Owns one uncached RGBA 5551 libdragon surface built from a cooked engine texture asset.
class Nintendo64RuntimeTexture final : public ::RuntimeTexture {
public:
    /// Creates an empty texture that owns no surface until `Initialize` succeeds.
    Nintendo64RuntimeTexture();
    /// Releases the surface through `Dispose`.
    ~Nintendo64RuntimeTexture() override;

    /// Allocates the surface and converts the cooked straight-RGBA payload; returns false when the asset is unusable.
    /// Any previously owned surface is released first, so a second call replaces the texture rather than leaking it;
    /// an unusable asset is rejected before that release, leaving an already initialized texture untouched, while a
    /// failed allocation leaves the texture with no surface and `Surface()` null.
    bool Initialize(const ::TextureAsset* asset);
    /// Surface consumed by the RDP blitter, or null before initialization.
    const surface_t* Surface() const { return SurfaceInitialized ? &SurfaceValue : nullptr; }
    /// Releases the surface and marks the texture disposed; safe to call more than once.
    void Dispose() override;

private:
    /// Frees any owned surface, clears the initialized flag and the reported dimensions, and leaves the
    /// disposal state alone; does nothing when no surface is owned, so it is safe to call at any time.
    /// This is the one place that frees the surface: both `Dispose` and `Initialize` go through it.
    void ReleaseSurface();

    /// libdragon surface holding the converted RGBA 5551 texels; only valid while `SurfaceInitialized` is true.
    surface_t SurfaceValue{};
    /// True once `SurfaceValue` owns an allocated buffer that `ReleaseSurface` must free.
    bool SurfaceInitialized = false;
};

} // namespace helengine::n64

#endif
