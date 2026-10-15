#include <cstdio>
#include <cstdlib>

#include <libdragon.h>
#include "platform/n64/Nintendo64InputBackend.hpp"

namespace {
joypad_inputs_t inputs[4];
joypad_style_t styles[4] = {JOYPAD_STYLE_N64, JOYPAD_STYLE_NONE, JOYPAD_STYLE_NONE, JOYPAD_STYLE_NONE};
int failures = 0;
void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}
}

void joypad_init() {}
void joypad_close() {}
void joypad_poll() {}
joypad_style_t joypad_get_style(joypad_port_t port) { return styles[port]; }
joypad_inputs_t joypad_get_inputs(joypad_port_t port) { return inputs[port]; }

int main() {
    helengine::n64::Nintendo64InputBackend backend;
    struct Case { int8_t x, y; int16_t expectedX, expectedY; };
    // libdragon is +Y up; engine/menu is -Y up. Horizontal remains -X left.
    const Case cases[] = {
        {-80, 80, -20480, -20480}, {80, -80, 20480, 20480},
        {0, 64, 0, -16384}, {0, -64, 0, 16384}, {0, 0, 0, 0},
        {127, 127, 32512, -32512}, {-127, -127, -32512, 32512},
        {-128, -128, -32768, 32767}
    };
    for (const auto& test : cases) {
        inputs[0].stick_x = test.x;
        inputs[0].stick_y = test.y;
        const auto frame = backend.CaptureFrame();
        const auto& pad = frame.Gamepads->Data[0];
        Check(pad.Connected, "raw N64 controller remains connected");
        Check(pad.LeftStickX == test.expectedX, "horizontal raw sign and scale stay unchanged");
        Check(pad.LeftStickY == test.expectedY, "vertical raw sign follows engine up-negative contract without overflow");
    }
    inputs[0].btn.a = true; inputs[0].btn.b = true;
    inputs[0].btn.d_up = true; inputs[0].btn.d_down = false;
    const auto frame = backend.CaptureFrame();
    const auto& pad = frame.Gamepads->Data[0];
    Check(pad.IsButtonDown(InputGamepadButton::South), "N64 A still maps to engine South");
    Check(pad.IsButtonDown(InputGamepadButton::East), "N64 B still maps to engine East");
    Check(pad.IsButtonDown(InputGamepadButton::DPadUp) && !pad.IsButtonDown(InputGamepadButton::DPadDown),
          "digital up/down mapping remains unchanged");
    Check(!frame.Gamepads->Data[1].Connected && frame.Gamepads->Data[1].LeftStickY == 0,
          "disconnected controller remains neutral");
    if (failures != 0) return EXIT_FAILURE;
    std::puts("n64-input-backend-tests: PASS");
    return EXIT_SUCCESS;
}
