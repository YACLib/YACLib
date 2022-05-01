#pragma once

namespace yaclib::detail {

template <typename Impl>
class Mutex {
 public:
 private:
  Impl _impl;
};

}  // namespace yaclib::detail
