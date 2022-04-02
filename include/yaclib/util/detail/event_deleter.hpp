#pragma once

#include <yaclib/config.hpp>

namespace yaclib::detail {

struct EventDeleter {
  template <typename Event>
  static void Delete(Event* event) {
    assert(event);
    event->Set();
  }
};

}  // namespace yaclib::detail
