#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "platform/n64/Nintendo64RuntimeTexture.hpp"

namespace helengine::n64 {

/// Tracks bank-zero TLUT contents only while one command list is executing; never owns texture pointers.
/// libdragon CI blits use the lower 2048 TMEM bytes, preserving the TLUT at 0x800. Direct RGBA16 blits
/// can use all 4096 bytes, so every direct bind invalidates the remembered palette before the next CI draw.
class Nintendo64TextureBindingCache {
public:
    /// Selects the texture's mode and reloads its palette only when bank zero is not already known to match.
    /// The executor calls this only after clipping and geometry validation, immediately before its actual blit.
    void Bind(const Nintendo64RuntimeTexture* texture) {
        if (texture == nullptr || texture->Surface() == nullptr) return;
        texture->Bind(LastIndexedTexture != texture);
        LastIndexedTexture = texture->IsIndexed() ? texture : nullptr;
    }

private:
    /// Borrowed for one Execute stack frame, during which resources are immutable and cannot be released.
    const Nintendo64RuntimeTexture* LastIndexedTexture = nullptr;
};

} // namespace helengine::n64

#endif
