#pragma once

#include "tdb.h"

#include <atomic>

namespace taco {

class alignas(CACHELINE_SIZE) SpinLock {
public:
    SpinLock(): locked_(false) {}

    void lock() noexcept {
        for (;;) {
            if (!locked_.exchange(true, std::memory_order_acquire)) {
                return;
            }

            while (locked_.load(std::memory_order_relaxed)) {
                __builtin_ia32_pause();
            }
        }
    }

    void unlock() noexcept {
        locked_.store(false, std::memory_order_release);
    }
private:
    std::atomic_bool locked_;
};

}