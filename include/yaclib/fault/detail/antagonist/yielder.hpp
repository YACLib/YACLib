#pragma once

// TODO(myannayx): define in cmake depending on system
#define SLEEP_TIME 63

#include <yaclib/fault/thread.hpp>

#include <atomic>
#include <random>

namespace yaclib::detail {
// TODO(myannyax) stats?
class Yielder {
 public:
  explicit Yielder(int frequency);
  void MaybeYield();

 private:
  bool ShouldYield();
  void Reset();
  unsigned RandNumber(unsigned max);

  ::std::atomic<int> _count;
  const unsigned _freq;
  ::std::mt19937 _eng;
};

}  // namespace yaclib::detail
