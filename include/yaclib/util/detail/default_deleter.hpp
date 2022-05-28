#pragma once

namespace yaclib::detail {

struct [[maybe_unused]] DefaultDeleter {
  template <typename Type>
  static void Delete(Type& self) noexcept {
    delete &self;
  }
};

}  // namespace yaclib::detail
