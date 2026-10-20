#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cstdint>

#include "RuntimeMaterial.hpp"

namespace helengine::n64 {
class Nintendo64RenderManager2D;
class Nintendo64RuntimeTexture;

/// Holds the cooked opaque color, culling, and lighting choice for one N64 material.
class Nintendo64RuntimeMaterial final : public ::RuntimeMaterial {
public:
    /// Creates one material from cooked color, culling, and lit flags.
    Nintendo64RuntimeMaterial(uint32_t color, bool doubleSided, bool lit);
    ~Nintendo64RuntimeMaterial() override;

    /// Transfers one uploaded texture to this material, retaining its allocation owner for release.
    void SetTexture(Nintendo64RenderManager2D& owner, Nintendo64RuntimeTexture* texture);
    /// Releases an owned texture once; its native disposal waits for queued DMA consumers.
    void Dispose() override;
    /// Borrowed texture used by the opaque mesh pass, or null for an untextured material.
    Nintendo64RuntimeTexture* Texture() const { return TextureValue; }

    /// Packed RGBA8 base color written into every vertex.
    uint32_t Color() const { return ColorValue; }
    /// True when back faces are drawn.
    bool DoubleSided() const { return DoubleSidedValue; }
    /// True when Tiny3D ambient and directional lighting modulate the base color.
    bool Lit() const { return LitValue; }

private:
    /// Packed RGBA8 base color taken from the cooked material.
    uint32_t ColorValue;
    /// Cooked back-face culling choice.
    bool DoubleSidedValue;
    /// Cooked lighting choice.
    bool LitValue;
    Nintendo64RenderManager2D* TextureOwner = nullptr;
    Nintendo64RuntimeTexture* TextureValue = nullptr;
    bool DisposedValue = false;
};

} // namespace helengine::n64

#endif
