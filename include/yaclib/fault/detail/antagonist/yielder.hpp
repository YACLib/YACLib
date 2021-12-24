#pragma once

// TODO(myannayx): define in cmake depending on system

#include <yaclib/fault/thread.hpp>

#include <atomic>
#include <random>

namespace yaclib::detail {

// TODO(myannyax) stats?
class Yielder {
 public:
  explicit Yielder(uint32_t frequency);
  void MaybeYield();

 private:
  bool ShouldYield();
  void Reset();
  unsigned RandNumber(uint32_t max);

  std::atomic_uint32_t _count;
  const uint32_t _freq;
  std::mt19937 _eng;
};

}  // namespace yaclib::detail
