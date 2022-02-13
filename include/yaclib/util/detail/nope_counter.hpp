#pragma once

namespace yaclib::detail {

template <typename CounterBase>
struct NopeCounter final : CounterBase {
  using CounterBase::CounterBase;

  void IncRef() noexcept final {
  }

  void DecRef() noexcept final {
  }
};

}  // namespace yaclib::detail
