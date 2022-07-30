#pragma once

namespace yaclib::detail {

template <typename... Bases>
struct NopeCounter : Bases... {
  void IncRef() noexcept final {
  }

  void DecRef() noexcept final {
  }
};

}  // namespace yaclib::detail
