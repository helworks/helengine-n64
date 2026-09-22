#pragma once

#include <cstdint>

/// <summary>
/// Supplies the N64 monotonic clock used by generated Stopwatch instances.
/// </summary>
extern "C" std::uint64_t get_ticks_us(void);

namespace he_cpp_custom {

/// <summary>
/// Returns libdragon's monotonic microsecond counter for generated timing services.
/// </summary>
inline std::uint64_t MonotonicMicroseconds() {
    return get_ticks_us();
}

} // namespace he_cpp_custom
