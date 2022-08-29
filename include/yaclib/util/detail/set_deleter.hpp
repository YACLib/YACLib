#pragma once

namespace yaclib::detail {

struct NopeBase {};

struct NopeDeleter final {
  template <typename Event>
  static void Delete(Event&) noexcept {
  }
};

struct SetDeleter final {
  template <typename Event>
  static void Delete(Event& event) noexcept {
    event.Set();
  }
};

}  // namespace yaclib::detail
