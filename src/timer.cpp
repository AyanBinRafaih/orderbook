#include <timer.h>
#include <ctime>

namespace timer {
    double calibrate_cycles_per_ns() {
        double measurements[5]; // Take 5 measurements and calculate the median
        for (int i = 0; i < 5; ++i){
            struct timespec ts_start, ts_end;
            clock_gettime(CLOCK_MONOTONIC_RAW, &ts_start);
            uint64_t tsc_start = start();

            struct timespec sleep_time = {0, 100'000'000}; // 100 ms
            nanosleep(&sleep_time, nullptr);

            uint64_t tsc_end = stop();
            clock_gettime(CLOCK_MONOTONIC_RAW, &ts_end);

            uint64_t tsc_delta = tsc_end - tsc_start;
            uint64_t wall_ns = (ts_end.tv_sec - ts_start.tv_sec) * 1'000'000'000ULL + (ts_end.tv_nsec - ts_start.tv_nsec);
            measurements[i] = static_cast<double>(tsc_delta) / static_cast<double>(wall_ns);
        }

        // Insertion sort
        for (int i = 0; i < 5; ++i){
            double key = measurements[i];
            int j = i - 1;
            while (j >= 0 && measurements[j] > key){
                measurements[j + 1] = measurements[j];
                --j;
            }
            measurements[j + 1] = key;
        }

        return measurements[2];
    }
}