#pragma once

#include <yaclib/algo/detail/unique_core.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename V, typename T>
class Promise final {
  static_assert(Check<V>(), "V should be valid");
  static_assert(!std::is_same_v<V, typename T::Error>,
                "V cannot be the same as the trait Error type, because callback dispatch would be ambiguous");

 public:
  Promise(const Promise& other) = delete;
  Promise& operator=(const Promise& other) = delete;

  Promise(Promise&& other) noexcept = default;
  Promise& operator=(Promise&& other) noexcept = default;

  /**
   * The default constructor creates not a \ref Valid Promise
   *
   * Needed only for usability, e.g. instead of std::optional<Promise<T>> in containers.
   */
  Promise() noexcept = default;

  /**
   * If Promise is \ref Valid then set \ref StopTag
   */
  ~Promise() noexcept {
    if (Valid()) {
      std::move(*this).Set(StopTag{});
    }
  }

  /**
   * Check if this \ref Promise has \ref Future
   *
   * \return false if this \ref Promise is default-constructed or moved to, otherwise true
   */
  [[nodiscard]] bool Valid() const& noexcept {
    return _core != nullptr;
  }

  /**
   * Set \ref Promise result
   *
   * \tparam Args \ref T::MakeResult<V> should be invocable with this types
   * \param args arguments
   */
  template <typename... Args>
  void Set(Args&&... args) && {
    YACLIB_ASSERT(Valid());
    if constexpr (sizeof...(Args) == 0) {
      _core->Store(Unit{});
    } else {
      _core->Store(std::forward<Args>(args)...);
    }
    auto* core = _core.Release();
    detail::Loop(core, core->template SetResult<false>());
  }

  /**
   * Part of unsafe but internal API
   */
  explicit Promise(detail::UniqueCorePtr<V, T> core) noexcept : _core{std::move(core)} {
  }

  [[nodiscard]] detail::UniqueCorePtr<V, T>& GetCore() noexcept {
    return _core;
  }

 private:
  detail::UniqueCorePtr<V, T> _core;
};

extern template class Promise<>;

}  // namespace yaclib
