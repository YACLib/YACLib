#pragma once

namespace yaclib::detail {

template <typename Impl>
class SharedTimedMutex {
 public:
 private:
  Impl _impl;
};

}  // namespace yaclib::detail
