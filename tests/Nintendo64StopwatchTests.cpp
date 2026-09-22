#include <cmath>
#include <cstdint>
#include <cstdio>

#include "system/diagnostics/stopwatch.hpp"

namespace {
uint64_t nowUs = 0;
int failures = 0;

void CheckMilliseconds(const Stopwatch& stopwatch, double expected, const char* message) {
    const double actual = stopwatch.Elapsed.TotalMilliseconds;
    if (std::fabs(actual - expected) > 0.000001) {
        std::fprintf(stderr, "FAIL: %s (expected %.6f, got %.6f)\n", message, expected, actual);
        ++failures;
    }
}
}

// Supply deterministic hardware readings to the real N64 runtime provider.
extern "C" uint64_t get_ticks_us(void) {
    return nowUs;
}

int main() {
    Stopwatch stopwatch;
    CheckMilliseconds(stopwatch, 0.0, "new stopwatch is stopped at zero");

    nowUs = 0xfffffff0ULL;
    stopwatch.Restart();
    nowUs += 1250;
    CheckMilliseconds(stopwatch, 1.25, "microseconds become milliseconds across the 32-bit boundary");
    stopwatch.Stop();
    nowUs += 500000;
    CheckMilliseconds(stopwatch, 1.25, "stopped timing does not include idle time");

    stopwatch.Start();
    nowUs += 2750;
    stopwatch.Stop();
    CheckMilliseconds(stopwatch, 4.0, "resume accumulates only running intervals");

    nowUs = 0x100000000000ULL;
    stopwatch.Restart();
    nowUs += 16667;
    stopwatch.Stop();
    CheckMilliseconds(stopwatch, 16.667, "restart discards old timing and retains a 64-bit clock");

    if (failures != 0) {
        return 1;
    }
    std::puts("n64-stopwatch-tests: PASS");
    return 0;
}
