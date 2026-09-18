#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cstdint>
#include <string>

#include "RenderCommandListBuilder2D.hpp"
#include "RenderManager2D.hpp"

#include "platform/n64/Nintendo64RenderCommandExecutor2D.hpp"

namespace helengine::n64 {

/// Screen-space 2D render manager for the Nintendo 64: turns cooked engine texture payloads into RDP-ready
/// surfaces and, once the 3D pass has finished the frame, flattens every authored camera's render queue
/// through the shared `RenderCommandListBuilder2D` and executes the result with `Nintendo64RenderCommandExecutor2D`.
class Nintendo64RenderManager2D final : public ::RenderManager2D {
public:
    /// Disposes and releases the retained command-list builder, if `Draw` ever allocated one.
    ~Nintendo64RenderManager2D() override;

    /// Builds one runtime texture from an already deserialized cooked engine texture payload.
    ::RuntimeTexture* BuildTextureFromRaw(::TextureAsset* data) override;
    /// Builds one runtime texture from a cooked engine texture payload read through the content source.
    ::RuntimeTexture* BuildTextureFromCooked(std::string cookedAssetPath, ::IContentStreamSource* contentStreamSource) override;
    /// Runs the screen-space 2D pass after the 3D pass for every authored camera.
    void Draw() override;
    /// Fails loudly. Generated Core only reaches this by visiting the 2D queue with this manager as the
    /// visitor; the N64 pass instead drives `Draw` from a command list built by `RenderCommandListBuilder2D`,
    /// so arriving here means the pass was driven the old way and the drawable would be silently dropped.
    void DrawRoundedRect(::IRoundedRectDrawable2D* shape) override;
    /// Fails loudly, for the same reason as `DrawRoundedRect`.
    void DrawSprite(::ISpriteDrawable2D* sprite) override;
    /// Fails loudly, for the same reason as `DrawRoundedRect`.
    void DrawText(::ITextDrawable2D* text) override;

protected:
    /// Rejects partial texture updates: N64 textures are built once from a cooked payload and never streamed into.
    void UpdateTextureRegionCore(::RuntimeTexture* texture, int32_t x, int32_t y, int32_t width, int32_t height,
                                 ::Array<uint8_t>* rgba8, int32_t sourceRowPitch) override;

private:
    /// Shared 2D command-list builder. It owns and reuses the `RenderCommandList2D` it returns across frames,
    /// so it is allocated lazily on the first `Draw` and never rebuilt; the destructor is the only place that
    /// frees it, because deleting the list it returns instead would free the builder's own state.
    ::RenderCommandListBuilder2D* CommandListBuilder = nullptr;
    /// Executes one flattened command list against the RDP.
    Nintendo64RenderCommandExecutor2D Executor;
};

} // namespace helengine::n64

#endif
