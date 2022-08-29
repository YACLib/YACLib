#pragma once

#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib {
namespace detail {

template <template <typename...> typename Counter, typename ObjectT>
class Helper final : public Counter<ObjectT, DefaultDeleter> {
 public:
  using Counter<ObjectT, DefaultDeleter>::Counter;

  void IncRef() noexcept final {
    this->Add(1);
  }

  void DecRef() noexcept final {
    this->Sub(1);
  }
};

}  // namespace detail

template <typename ObjectT, typename... Args>
auto MakeUnique(Args&&... args) {
  return IntrusivePtr{NoRefTag{}, new detail::Helper<detail::OneCounter, ObjectT>{0, std::forward<Args>(args)...}};
}

template <typename ObjectT, typename... Args>
auto MakeShared(std::size_t n, Args&&... args) {
  return IntrusivePtr{NoRefTag{}, new detail::Helper<detail::AtomicCounter, ObjectT>{n, std::forward<Args>(args)...}};
}

}  // namespace yaclib
