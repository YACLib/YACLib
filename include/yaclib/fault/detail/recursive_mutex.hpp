#pragma once

#include <yaclib/fault/detail/mutex.hpp>

namespace yaclib::detail {

// Not using because maybe in future we will want different types
template <typename Impl>
class RecursiveMutex : public Mutex<Impl> {
  using Base = Mutex<Impl>;

 public:
  using Base::Base;
};

}  // namespace yaclib::detail
