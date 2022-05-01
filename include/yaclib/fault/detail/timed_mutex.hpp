#pragma once

namespace yaclib::detail {

template <typename Impl>
class TimedMutex {
 public:
 private:
  Impl _impl;
};

}  // namespace yaclib::detail
