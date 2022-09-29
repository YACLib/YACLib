#pragma once

#include <yaclib/config.hpp>

#include <atomic>
#include <random>

namespace yaclib::detail {

void SetSeed(std::uint32_t new_seed);

std::uint32_t GetSeed();

std::uint64_t GetRandNumber(std::uint64_t max);

std::uint64_t GetRandCount();

void ForwardToRandCount(std::uint64_t random_count);

}  // namespace yaclib::detail
