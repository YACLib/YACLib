#pragma once

namespace yaclib::detail {

struct SetDeleter final {
  template <typename Event>
  static void Delete(Event& event) noexcept {
    event.Set();
  }
};

}  // namespace yaclib::detail
