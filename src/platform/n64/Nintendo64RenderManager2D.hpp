#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "RenderManager2D.hpp"

namespace helengine::n64 {

class Nintendo64RenderManager2D final : public ::RenderManager2D {
public:
    ::RuntimeTexture* BuildTextureFromRaw(::TextureAsset* data) override;
    void DrawRoundedRect(::IRoundedRectDrawable2D* shape) override;
    void DrawSprite(::ISpriteDrawable2D* sprite) override;
    void DrawText(::ITextDrawable2D* text) override;

protected:
    void UpdateTextureRegionCore(::RuntimeTexture* texture, int32_t x, int32_t y, int32_t width, int32_t height,
                                 ::Array<uint8_t>* rgba8, int32_t sourceRowPitch) override;
};

} // namespace helengine::n64

#endif
