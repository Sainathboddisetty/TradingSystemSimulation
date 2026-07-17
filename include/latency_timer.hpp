#pragma once
#include "types.hpp"
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <array>
#include <numeric>
#include <chrono>
#include <thread>

// ============================================================
// Latency Measurement with RDTSC
//
// ULL NOTE: std::chrono::high_resolution_clock takes ~20-40ns
// itself — too slow to measure a 100ns event accurately.
// clock_gettime(CLOCK_MONOTONIC) requires a vDSO call: ~5-15ns.
//
// RDTSC (Read Time Stamp Counter) reads the CPU cycle counter
// in ~1-3ns. We convert cycles → nanoseconds using the CPU
// frequency (measured at startup or read from /proc/cpuinfo).
//
// RDTSCP adds a serialising instruction so speculative execution
// doesn't reorder the read. Use RDTSCP for start/stop pairs.
//
// WARNING: RDTSC is per-core. If your thread migrates between
// cores during a measurement the delta is garbage. In production:
//   - Pin threads to cores with CPU affinity (sched_setaffinity)
//   - Use RDTSCP + LFENCE/CPUID as serialising fences
// ============================================================

namespace ts {

// Read the CPU timestamp counter (non-serialising, use for bulk sampling)
inline uint64_t rdtsc() noexcept {
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

// Serialising version — prevents reordering across the fence
inline uint64_t rdtscp() noexcept {
    uint32_t lo, hi, aux;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

// Estimate CPU frequency by comparing RDTSC against a known sleep.
// Call once at startup. Returns cycles per nanosecond.
inline double cpu_ghz() {
    using namespace std::chrono;

    auto start = steady_clock::now();
    uint64_t t0 = rdtscp();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    uint64_t t1 = rdtscp();
    auto end = steady_clock::now();

    auto elapsed_ns = duration_cast<nanoseconds>(end - start).count();

    return static_cast<double>(t1 - t0) / elapsed_ns;
}

// Convert RDTSC delta to nanoseconds
inline double ticks_to_ns(uint64_t ticks, double ghz) noexcept {
    return static_cast<double>(ticks) / ghz;
}

// ============================================================
// LatencyStats — rolling statistics over a fixed sample window
// ============================================================
template<std::size_t N = 1024>
class LatencyStats {
    std::array<double, N> samples_{};
    std::size_t count_{0};
    double ghz_;

public:
    explicit LatencyStats(double ghz) : ghz_(ghz) {}

    void record(uint64_t start_ticks, uint64_t end_ticks) {
        samples_[count_ % N] = ticks_to_ns(end_ticks - start_ticks, ghz_);
        ++count_;
    }

    void print_summary(const char* label) const {
        if (count_ == 0) { std::printf("%s: no samples\n", label); return; }
        std::size_t n = std::min(count_, N);
        auto begin = samples_.begin();
        auto end   = samples_.begin() + n;

        std::array<double, N> sorted{};
        std::copy(begin, end, sorted.begin());
        std::sort(sorted.begin(), sorted.begin() + n);

        double sum = std::accumulate(begin, end, 0.0);
        double avg = sum / static_cast<double>(n);
        double p50 = sorted[n * 50 / 100];
        double p99 = sorted[n * 99 / 100];
        double p999= sorted[n * 999 / 1000 < n ? n * 999 / 1000 : n - 1];

        std::printf("\n--- Latency: %s (n=%zu) ---\n", label, n);
        std::printf("  avg=%.1f ns   p50=%.1f ns   p99=%.1f ns   p99.9=%.1f ns\n",
            avg, p50, p99, p999);
    }
};

} 
