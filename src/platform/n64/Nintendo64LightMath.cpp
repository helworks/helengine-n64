#include "Nintendo64LightMath.hpp"

#include <cmath>

namespace helengine::n64 {
namespace {

/// Clamps one linear channel to [0,1] and rounds it to an 8-bit value.
uint8_t ToByte(float channel) {
    const float clamped = channel < 0.0f ? 0.0f : (channel > 1.0f ? 1.0f : channel);
    return static_cast<uint8_t>(std::lround(clamped * 255.0f));
}

/// True when every channel of an RGB triple is finite and can therefore be clamped and rounded.
bool IsFiniteColor(float red, float green, float blue) {
    return std::isfinite(red) && std::isfinite(green) && std::isfinite(blue);
}

} // namespace

bool Nintendo64LightMath::PackColor(float red, float green, float blue, float intensity, uint8_t output[4]) {
    const float scaledRed = red * intensity;
    const float scaledGreen = green * intensity;
    const float scaledBlue = blue * intensity;
    if (!IsFiniteColor(scaledRed, scaledGreen, scaledBlue)) {
        return false;
    }
    output[0] = ToByte(scaledRed);
    output[1] = ToByte(scaledGreen);
    output[2] = ToByte(scaledBlue);
    output[3] = 255;
    return true;
}

void Nintendo64LightMath::Accumulate(float red, float green, float blue, float intensity, float accumulator[3]) {
    accumulator[0] += red * intensity;
    accumulator[1] += green * intensity;
    accumulator[2] += blue * intensity;
}

bool Nintendo64LightMath::PackAccumulated(const float accumulator[3], uint8_t output[4]) {
    if (!IsFiniteColor(accumulator[0], accumulator[1], accumulator[2])) {
        return false;
    }
    output[0] = ToByte(accumulator[0]);
    output[1] = ToByte(accumulator[1]);
    output[2] = ToByte(accumulator[2]);
    output[3] = 255;
    return true;
}

bool Nintendo64LightMath::ToLightVector(float forwardX, float forwardY, float forwardZ, float output[3]) {
    const float lengthSquared = forwardX * forwardX + forwardY * forwardY + forwardZ * forwardZ;
    if (!(lengthSquared > 0.0f) || !std::isfinite(lengthSquared)) {
        return false;
    }
    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    output[0] = -forwardX * inverseLength;
    output[1] = -forwardY * inverseLength;
    output[2] = -forwardZ * inverseLength;
    return true;
}

} // namespace helengine::n64
