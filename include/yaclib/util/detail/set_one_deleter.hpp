#pragma once

namespace yaclib::detail {

struct SetOneDeleter {
  template <typename Event>
  static void Delete(Event& event) {
    event.SetOne();
  }
};

}  // namespace yaclib::detail
