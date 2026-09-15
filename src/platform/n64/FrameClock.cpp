#include "FrameClock.hpp"

namespace helengine::n64 {

void FrameClock::Reset(uint64_t nowUs) {
    PreviousUs = nowUs;
    AngleXRadians = 0.0f;
    AngleYRadians = 0.0f;
    HasPrevious = true;
}

uint32_t FrameClock::Advance(uint64_t nowUs) {
    if (!HasPrevious) {
        Reset(nowUs);
        return 0;
    }

    uint64_t deltaUs = 0;
    if (nowUs >= PreviousUs) {
        deltaUs = nowUs - PreviousUs;
    }
    PreviousUs = nowUs;

    if (deltaUs > MaxDeltaUs) {
        deltaUs = MaxDeltaUs;
    }

    const float seconds = static_cast<float>(deltaUs) * 1.0e-6f;
    AngleXRadians += seconds * XRadiansPerSecond;
    AngleYRadians += seconds * YRadiansPerSecond;
    while (AngleXRadians >= FullTurnRadians) {
        AngleXRadians -= FullTurnRadians;
    }
    while (AngleYRadians >= FullTurnRadians) {
        AngleYRadians -= FullTurnRadians;
    }
    return static_cast<uint32_t>(deltaUs);
}

} // namespace helengine::n64
