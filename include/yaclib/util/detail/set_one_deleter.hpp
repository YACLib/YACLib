#pragma once

namespace yaclib::detail {

struct SetOneDeleter final {
  template <typename Event>
  static void Delete(Event& event) noexcept {
    event.SetOne();
  }
};

}  // namespace yaclib::detail
