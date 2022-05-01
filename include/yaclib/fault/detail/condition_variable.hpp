#pragma once

#include <yaclib_std/mutex>

namespace yaclib::detail {

template <typename Impl>
class ConditionVariable {
 public:
 private:
  Impl _impl;
};

}  // namespace yaclib::detail
