#pragma once

#include <yaclib/fault/detail/timed_mutex.hpp>

namespace yaclib::detail {

// Not using because maybe in future we will want different types
template <typename Impl>
class RecursiveTimedMutex : public TimedMutex<Impl> {
  using Base = TimedMutex<Impl>;

 public:
  using Base::Base;
};

}  // namespace yaclib::detail
