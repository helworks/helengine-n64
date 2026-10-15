#pragma once

#include <cstdint>
#include <vector>

template <typename T> struct Array {
    explicit Array(int count) : Data(count) {}
    std::vector<T> Data;
};

enum class InputGamepadButton {
    South, East, West, North, LeftShoulder, RightShoulder,
    LeftTrigger, Start, DPadUp, DPadDown, DPadLeft, DPadRight
};

struct InputGamepadState {
    bool Connected = false;
    int16_t LeftStickX = 0;
    int16_t LeftStickY = 0;
    int16_t LeftTrigger = 0;
    uint32_t Buttons = 0;
    void SetButtonDown(InputGamepadButton button, bool down) {
        const uint32_t mask = uint32_t{1} << static_cast<unsigned>(button);
        Buttons = down ? Buttons | mask : Buttons & ~mask;
    }
    bool IsButtonDown(InputGamepadButton button) const {
        return (Buttons & (uint32_t{1} << static_cast<unsigned>(button))) != 0;
    }
};

struct InputFrameState {
    Array<InputGamepadState>* Gamepads = nullptr;
    int GamepadCount = 0;
};

class IInputBackend {
public:
    virtual ~IInputBackend() = default;
    virtual InputFrameState CaptureFrame() = 0;
};
