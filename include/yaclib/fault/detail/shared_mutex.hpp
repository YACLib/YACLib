#pragma once

namespace yaclib::detail {

template <typename Impl>
class SharedMutex {
 public:
 private:
  Impl _impl;
};

}  // namespace yaclib::detail
