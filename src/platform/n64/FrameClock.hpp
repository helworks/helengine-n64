#pragma once

#include <cstdint>

namespace helengine::n64 {

class FrameClock {
public:
    static constexpr uint64_t MaxDeltaUs = 100'000;

    void Reset(uint64_t nowUs);
    uint32_t Advance(uint64_t nowUs);

    float AngleX() const { return AngleXRadians; }
    float AngleY() const { return AngleYRadians; }

private:
    static constexpr float XRadiansPerSecond = 0.35f;
    static constexpr float YRadiansPerSecond = 0.55f;
    static constexpr float FullTurnRadians = 6.28318530718f;

    uint64_t PreviousUs = 0;
    float AngleXRadians = 0.0f;
    float AngleYRadians = 0.0f;
    bool HasPrevious = false;
};

} // namespace helengine::n64
