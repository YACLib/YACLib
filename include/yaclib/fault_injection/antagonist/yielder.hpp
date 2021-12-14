#pragma once

#include <yaclib/fault_injection/thread/this_thread.hpp>

#include <atomic>
#include <random>

namespace yaclib::std {

// TODO(myannyax) stats?
class Yielder {
 public:
  explicit Yielder(int frequency);
  void MaybeYield();

 private:
  bool ShouldYield();
  void Reset();

  ::std::atomic<int> _count;
  int _freq;
  ::std::mt19937 _eng;
};

}  // namespace yaclib::std
