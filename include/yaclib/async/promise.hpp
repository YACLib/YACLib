#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename V, typename E = StopError>
class [[nodiscard]] Promise final {
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
  ~Promise() noexcept;

  /**
   * Check if this \ref Promise has \ref Future
   *
   * \return false if this \ref Promise is default-constructed or moved to, otherwise true
   */
  [[nodiscard]] bool Valid() const& noexcept;

  /**
   * Set \ref Promise result
   *
   * \tparam T \ref Result<T> should be constructable from this type
   * \param value value
   */
  template <typename T>
  void Set(T&& value) && /*TODO noexcept*/;

  /**
   * Set \ref Promise<void> result
   */
  void Set() && noexcept;

  /**
   * Part of unsafe but internal API
   */
  explicit Promise(detail::ResultCorePtr<V, E> core) noexcept;
  [[nodiscard]] detail::ResultCorePtr<V, E>& GetCore() noexcept;

 private:
  detail::ResultCorePtr<V, E> _core;
};

extern template class Promise<void, StopError>;

}  // namespace yaclib

#include <yaclib/async/detail/promise_impl.hpp>
