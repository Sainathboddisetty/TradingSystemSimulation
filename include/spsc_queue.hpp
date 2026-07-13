#pragma once
#include <atomic>
#include <array>
#include <cstddef>

// ============================================================
// Single-Producer Single-Consumer (SPSC) Lock-Free Ring Buffer
//
// Zero locks, zero kernel
// calls — both sides spin-poll using atomics.
//
// Why SPSC and not MPMC?
//   - One writer (exchange gateway) + one reader (strategy) is
//     the dominant pattern on the hot path.
//   - SPSC needs only two cache lines of shared state:
//     write_idx_ and read_idx_. MPMC needs CAS loops.
//
// Memory ordering:
//   write_idx_ store → memory_order_release   : "all writes to
//       buffer_[w] are visible before the index advances"
//   read_idx_ load   → memory_order_acquire   : "see all stores
//       published before the index was advanced"
//   Relaxed loads of own index: only this thread changes it.
//
// Cache line padding (alignas(64)):
//   write_idx_ and read_idx_ live on separate cache lines.
//   Without this, the two cores invalidate each other's line on
//   every update — "false sharing" — killing throughput.
//
// Capacity MUST be a power of 2 so (idx & mask) replaces modulo.
// ============================================================


namespace ts {

template<typename T, std::size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0,
        "SPSCQueue: Capacity must be a power of 2");

    static constexpr std::size_t kMask = Capacity - 1;

    alignas(64) std::atomic<std::size_t> write_idx_{0};
    alignas(64) std::atomic<std::size_t> read_idx_{0};
    alignas(64) std::array<T, Capacity>  buffer_{};

public:
    // Called by the PRODUCER thread only.
    // Returns false if the queue is full (back-pressure).
    bool push(const T& item) noexcept {
        const std::size_t w    = write_idx_.load(std::memory_order_relaxed);
        const std::size_t next = (w + 1) & kMask;

        if (__builtin_expect(next == read_idx_.load(std::memory_order_acquire), 0))
            return false; // full

        buffer_[w] = item;
        write_idx_.store(next, std::memory_order_release);
        return true;
    }

    // Called by the CONSUMER thread only.
    // Returns false if the queue is empty.
    bool pop(T& item) noexcept {
        const std::size_t r = read_idx_.load(std::memory_order_relaxed);

        if (__builtin_expect(r == write_idx_.load(std::memory_order_acquire), 1))
            return false; // empty

        item = buffer_[r];
        read_idx_.store((r + 1) & kMask, std::memory_order_release);
        return true;
    }

    bool empty() const noexcept {
        return read_idx_.load(std::memory_order_acquire)
            == write_idx_.load(std::memory_order_acquire);
    }

    std::size_t size() const noexcept {
        const std::size_t w = write_idx_.load(std::memory_order_acquire);
        const std::size_t r = read_idx_.load(std::memory_order_acquire);
        return (w - r) & kMask;
    }
};

} 
