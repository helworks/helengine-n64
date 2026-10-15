#pragma once

namespace helengine::n64 {

/// Clears existing N64 attachments only. A camera's stencil-clear request is a no-op because
/// this framebuffer has no stencil plane; it does not prevent valid color/depth rendering.
template<class VideoType, class ClearSettingsType, class ChannelConverter>
void ApplyCameraAttachmentClears(VideoType& video, ClearSettingsType settings, ChannelConverter channel) {
    if (settings.get_ClearColorEnabled()) {
        auto color = settings.get_ClearColor();
        video.SetClearColor(channel(color.X), channel(color.Y), channel(color.Z), channel(color.W));
    }
    video.ClearAttachments(settings.get_ClearColorEnabled(), settings.get_ClearDepthEnabled(), settings.get_ClearDepth());
}

/// Visits the current sorted camera list without retaining camera pointers across frames.
/// Each camera finishes its 3D and 2D queues before the next camera may clear or overlay it.
template<class CameraList, class ClearPass, class Draw3DPass, class Draw2DPass>
void RenderOrderedCameraPasses(CameraList& cameras, ClearPass clear, Draw3DPass draw3D, Draw2DPass draw2D) {
    for (int index = 0; index < cameras.get_Count(); ++index) {
        auto* camera = cameras.get_Item(index);
        if (camera == nullptr) continue;
        clear(camera);
        draw3D(camera);
        draw2D(camera);
    }
}

} // namespace helengine::n64
