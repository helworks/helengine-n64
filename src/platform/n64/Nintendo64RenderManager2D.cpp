#include "platform/n64/Nintendo64RenderManager2D.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "runtime/native_exceptions.hpp"

namespace helengine::n64 {

::RuntimeTexture* Nintendo64RenderManager2D::BuildTextureFromRaw(::TextureAsset*) {
    throw new NotSupportedException("N64 authored scenes do not use textures.");
}

void Nintendo64RenderManager2D::DrawRoundedRect(::IRoundedRectDrawable2D*) {
}

void Nintendo64RenderManager2D::DrawSprite(::ISpriteDrawable2D*) {
}

void Nintendo64RenderManager2D::DrawText(::ITextDrawable2D*) {
}

void Nintendo64RenderManager2D::UpdateTextureRegionCore(::RuntimeTexture*, int32_t, int32_t, int32_t, int32_t,
                                                        ::Array<uint8_t>*, int32_t) {
    throw new NotSupportedException("N64 authored scenes do not update textures.");
}

} // namespace helengine::n64

#endif
