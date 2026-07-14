#pragma once
#include <array>
#include <cstddef>
#include <cassert>

// ============================================================
// Fixed-Size Object Memory Pool
//
// ULL NOTE: malloc/new in the hot path is catastrophic:
//   - glibc malloc takes ~50-200ns per call
//   - It acquires a mutex internally (even in single-thread mode)
//   - It can trigger mmap / brk syscalls under the hood
//
// A memory pool pre-allocates all storage at startup.
// allocate() is O(1) and takes ~5ns — just a pointer pop.
//
// The storage array is a single contiguous block, which means:
//   - No fragmentation
//   - Prefetcher can walk it sequentially
//   - All objects fit in L1/L2 cache together (for small pools)
//
// IMPORTANT: This pool is NOT thread-safe. Each thread must own
// its own pool instance, or you must add a spinlock (rare in ULL).
// ============================================================

namespace ts {

template<typename T, std::size_t PoolSize>
class MemoryPool {
    // Raw storage — no constructors called yet
    alignas(alignof(T)) std::array<std::byte, sizeof(T) * PoolSize> storage_;

    // Stack of free pointers — O(1) alloc/free
    std::array<T*, PoolSize> free_list_;
    std::size_t free_count_{PoolSize};

public:
    MemoryPool() {
        for (std::size_t i = 0; i < PoolSize; ++i)
            free_list_[i] = reinterpret_cast<T*>(&storage_[i * sizeof(T)]);
    }

    // Allocate one object slot. Caller must placement-new into it.
    [[nodiscard]] T* allocate() noexcept {
        if (__builtin_expect(free_count_ == 0, 0)) return nullptr;
        return free_list_[--free_count_];
    }

    // Return a slot to the pool. Does NOT call destructor.
    void deallocate(T* ptr) noexcept {
        assert(free_count_ < PoolSize && "MemoryPool: double-free or over-return");
        free_list_[free_count_++] = ptr;
    }

    std::size_t available() const noexcept { return free_count_; }
    std::size_t capacity()  const noexcept { return PoolSize; }
};

} // namespace ts
