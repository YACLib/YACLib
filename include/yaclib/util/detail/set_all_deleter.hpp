#pragma once

namespace yaclib::detail {

struct SetAllDeleter {
  template <typename Event>
  static void Delete(Event& event) {
    event.SetAll();
  }
};

}  // namespace yaclib::detail
