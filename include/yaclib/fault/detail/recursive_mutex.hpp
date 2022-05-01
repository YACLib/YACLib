#pragma once

namespace yaclib::detail {

template <typename Impl>
class RecursiveMutex {
 public:
 private:
  Impl _impl;
};

}  // namespace yaclib::detail
