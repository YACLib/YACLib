#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fwd.hpp>

#include <type_traits>
#include <utility>

namespace yaclib::detail {

template <typename Func>
class FuncCore {
 protected:
  using Storage = std::decay_t<Func>;
  using Invoke = std::conditional_t<std::is_function_v<std::remove_reference_t<Func>>, Storage, Func>;

  // Func not template parameter of ctor, it's intentionally
  explicit FuncCore(Func&& f) {
    new (&_func.storage) Storage{std::forward<Func>(f)};
  }

  union State {
    YACLIB_NO_UNIQUE_ADDRESS Unit stub;
    YACLIB_NO_UNIQUE_ADDRESS Storage storage;

    State() noexcept : stub{} {
    }

    ~State() noexcept {
    }
  };
  YACLIB_NO_UNIQUE_ADDRESS State _func;
};

}  // namespace yaclib::detail
