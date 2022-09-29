#pragma once

#include <cstdint>

namespace yaclib {

/**
 * Sets frequency with which fault will be injected.
 * Default is 16.
 */
void SetFaultFrequency(std::uint32_t freq) noexcept;

/**
 * Sets seed for random, which will be used when deciding when to yield, for fibers scheduler and random wrapper for
 * tests. Default is 1239.
 */
void SetSeed(std::uint32_t seed) noexcept;

/**
 * Sets sleep time if sleep is used instead of yield for interrupting thread execution for fault injection.
 * Default is 200
 */
void SetFaultSleepTime(std::uint32_t ns) noexcept;

std::uint32_t GetFaultSleepTime() noexcept;

/**
 * Sets frequency with which compare_exchange_weak would fail.
 * Default is 13
 */
void SetAtomicFailFrequency(std::uint32_t k) noexcept;

namespace fiber {

/**
 * Sets the amount of time to be added to fiber's scheduler time after each schedule cycle.
 * Default is 10
 */
void SetFaultTickLength(std::uint32_t ns) noexcept;

/**
 * Sets the length of scheduler queue prefix and suffix from which the next schedule candidate will be chosen.
 * Default is 10.
 */
void SetFaultRandomListPick(std::uint32_t k) noexcept;

/**
 * Sets fiber stack size for fault injection in pages.
 * Default is 8.
 */
void SetStackSize(std::uint32_t pages) noexcept;

/**
 * Sets fiber stack cache size for fault injection.
 * Default is 100.
 */
void SetStackCacheSize(std::uint32_t c) noexcept;

/**
 * Sets hardware_concurrency in fiber based execution.
 * Default equals to std::thread::hardware_concurrency.
 */
void SetHardwareConcurrency(std::uint32_t c) noexcept;

/**
 * @return current random count for fiber-based execution
 */
std::uint64_t GetFaultRandomCount() noexcept;

/**
 * Forwards random count for fiber-based execution to supplied one
 */
void ForwardToFaultRandomCount(std::uint64_t random_count) noexcept;

/**
 * @return current injector state
 */
std::uint32_t GetInjectorState() noexcept;

/**
 * Sets injector state to the supplied one
 */
void SetInjectorState(std::uint32_t state) noexcept;

}  // namespace fiber
}  // namespace yaclib
