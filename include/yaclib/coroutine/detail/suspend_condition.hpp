#pragma once

#include <yaclib/coroutine/detail/coroutine.hpp>

namespace yaclib::detail {

class SuspendCondition {
 public:
  SuspendCondition(bool condition) noexcept : _condition(condition) {
  }

  bool await_ready() noexcept {
    return _condition;
  }
  void await_resume() noexcept {
  }
  void await_suspend(yaclib_std::coroutine_handle<>) noexcept {
  }

 private:
  bool _condition;
};

}  // namespace yaclib::detail
