#pragma once
#include <cstdint>

namespace timer {
    // Reading CPU time-stamp counter at the start of an interval
    inline uint64_t start() {
        uint32_t lo, hi;
        __asm__ __volatile__ (
            "lfence\n\t"
            "rdtsc\n\t"
            : "=a"(lo), "=d"(hi)
        );
        return (static_cast<uint64_t>(hi) << 32) | lo;
    }

    // Reading the time-stamp counter at the end of an interval
    inline uint64_t stop() {
        uint32_t lo, hi;
        uint32_t aux; // Processor ID written by RDTSCP
        __asm__ __volatile__(
            "rdtscp\n\t"
            "lfence\n\t"
            : "=a"(lo), "=d"(hi), "=c"(aux)
        );
        return (static_cast<uint64_t>(hi) << 32) | lo;
    }

    // Measuring the number of time-stamp counter cycles occuring per nanosecond (sleep time ~ 0.5s)
    double calibrate_cycles_per_ns();

    // Convert cycle counts into nanoseconds using the calibration value from previously linked function
    inline double cycles_to_ns(uint64_t cycles, double cycles_per_ns){
        return static_cast<double>(cycles) / cycles_per_ns;
    }
}