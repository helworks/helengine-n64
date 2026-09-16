#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cstdint>
#include <string>

#include "RenderManager2D.hpp"
#include "IRenderVisitor2D.hpp"

namespace helengine::n64 {

/// Screen-space 2D render manager for the Nintendo 64: turns cooked engine texture payloads into RDP-ready
/// surfaces and blits the authored sprite queue of the first camera once the 3D pass has finished the frame.
class Nintendo64RenderManager2D final : public ::RenderManager2D, public ::IRenderVisitor2D {
public:
    /// Builds one runtime texture from an already deserialized cooked engine texture payload.
    ::RuntimeTexture* BuildTextureFromRaw(::TextureAsset* data) override;
    /// Builds one runtime texture from a cooked engine texture payload read through the content source.
    ::RuntimeTexture* BuildTextureFromCooked(std::string cookedAssetPath, ::IContentStreamSource* contentStreamSource) override;
    /// Runs the screen-space 2D pass after the 3D pass for the first authored camera.
    void Draw() override;
    /// Rounded rectangles are not part of the authored N64 2D feature set yet, so drawing one does nothing.
    void DrawRoundedRect(::IRoundedRectDrawable2D* shape) override;
    /// Draws one authored sprite as a textured screen-space rectangle.
    void DrawSprite(::ISpriteDrawable2D* sprite) override;
    /// Text is not part of the authored N64 2D feature set yet, so drawing a string does nothing.
    void DrawText(::ITextDrawable2D* text) override;
    /// Visits one queued 2D drawable and dispatches it back through generated Core.
    void Visit(::IDrawable2D* drawable) override;

protected:
    /// Rejects partial texture updates: N64 textures are built once from a cooked payload and never streamed into.
    void UpdateTextureRegionCore(::RuntimeTexture* texture, int32_t x, int32_t y, int32_t width, int32_t height,
                                 ::Array<uint8_t>* rgba8, int32_t sourceRowPitch) override;

private:
    /// Number of sprite blits emitted so far in the current frame's 2D pass; reset at the top of every `Draw`.
    int32_t DrawCallCount = 0;
    /// True once the first sprite blit of the run has been traced, so `n64:first-sprite-draw` is emitted exactly once.
    bool HasLoggedSpriteDraw = false;
};

} // namespace helengine::n64

#endif
