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
    bool Initialize(const ::TextureAsset* asset);
    /// Surface consumed by the RDP blitter, or null before initialization.
    const surface_t* Surface() const { return SurfaceInitialized ? &SurfaceValue : nullptr; }
    /// Frees the surface, clears the reported dimensions, and is safe to call more than once.
    void Dispose() override;

private:
    /// libdragon surface holding the converted RGBA 5551 texels; only valid while `SurfaceInitialized` is true.
    surface_t SurfaceValue{};
    /// True once `SurfaceValue` owns an allocated buffer that `Dispose` must free.
    bool SurfaceInitialized = false;
};

} // namespace helengine::n64

#endif
