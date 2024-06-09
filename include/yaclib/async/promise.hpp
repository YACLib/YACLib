#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename V, typename E>
class Promise final {
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>, "Promise cannot be instantiated with same V and E, because it's ambiguous");

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
   * \tparam Args \ref Result<T> should be constructable from this types
   * \param args arguments
   */
  template <typename... Args>
  void Set(Args&&... args) && {
    YACLIB_ASSERT(Valid());
    if constexpr (sizeof...(Args) == 0) {
      _core->Store(std::in_place);
    } else {
      _core->Store(std::forward<Args>(args)...);
    }
    auto* core = _core.Release();
    detail::Loop(core, core->template SetResult<false>());
  }

  /**
   * Part of unsafe but internal API
   */
  explicit Promise(detail::ResultCorePtr<V, E> core) noexcept : _core{std::move(core)} {
  }

  [[nodiscard]] detail::ResultCorePtr<V, E>& GetCore() noexcept {
    return _core;
  }

 private:
  detail::ResultCorePtr<V, E> _core;
};

extern template class Promise<>;

}  // namespace yaclib
