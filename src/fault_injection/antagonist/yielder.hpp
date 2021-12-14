#pragma once

#include "../thread/this_thread.hpp"

#include <atomic>
#include <random>

namespace yaclib::std {

// TODO(myannyax) stats?
class Yielder {
 public:
  void MaybeYield();

 private:
  bool ShouldYield();
  void Reset();

  ::std::atomic<int> _count;
  int _freq;
  ::std::mt19937 _eng;
};

}  // namespace yaclib::std
