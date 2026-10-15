#include "platform/n64/Nintendo64InputBackend.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "InputFrameState.hpp"
#include "InputGamepadButton.hpp"
#include <libdragon.h>

namespace helengine::n64 {
Nintendo64InputBackend::Nintendo64InputBackend() { joypad_init(); }
Nintendo64InputBackend::~Nintendo64InputBackend() { joypad_close(); }

::InputFrameState Nintendo64InputBackend::CaptureFrame() {
    ::InputFrameState frame;
    joypad_poll();
    auto& pads = ((++CapturedFrames) & 1) ? FrameA : FrameB;
    for (int i = 0; i < 4; ++i) {
        auto& pad = pads.Data[i];
        pad = ::InputGamepadState();
        auto port = static_cast<joypad_port_t>(i);
        if (joypad_get_style(port) == JOYPAD_STYLE_NONE) continue;
        auto input = joypad_get_inputs(port);
        pad.Connected = true;
        pad.LeftStickX = static_cast<int16_t>(input.stick_x * 256);
        // libdragon is positive-up; the engine's pad contract is negative-up.
        // Clamp raw -128 after inversion because +32768 cannot fit in int16_t.
        const int32_t engineStickY = -static_cast<int32_t>(input.stick_y) * 256;
        pad.LeftStickY = static_cast<int16_t>(engineStickY > 32767 ? 32767 : engineStickY);
        const auto button = [&](::InputGamepadButton value, bool down) { pad.SetButtonDown(value, down); };
        button(::InputGamepadButton::South, input.btn.a); button(::InputGamepadButton::East, input.btn.b);
        button(::InputGamepadButton::West, input.btn.c_left); button(::InputGamepadButton::North, input.btn.c_up);
        button(::InputGamepadButton::LeftShoulder, input.btn.l); button(::InputGamepadButton::RightShoulder, input.btn.r);
        button(::InputGamepadButton::LeftTrigger, input.btn.z); pad.LeftTrigger = input.btn.z ? 32767 : 0;
        button(::InputGamepadButton::Start, input.btn.start);
        button(::InputGamepadButton::DPadUp, input.btn.d_up); button(::InputGamepadButton::DPadDown, input.btn.d_down);
        button(::InputGamepadButton::DPadLeft, input.btn.d_left); button(::InputGamepadButton::DPadRight, input.btn.d_right);
    }
    frame.Gamepads = &pads;
    frame.GamepadCount = 4;
    return frame;
}

} // namespace helengine::n64

#endif
