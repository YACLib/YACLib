#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename V, typename E = StopError>
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
  Promise() = default;

  /**
   * If Promise is \ref Valid then set \ref StopTag
   */
  ~Promise();

  /**
   * Check if this \ref Promise has \ref Future
   *
   * \return false if this \ref Promise is default-constructed or moved to, otherwise true
   */
  [[nodiscard]] bool Valid() const& noexcept;

  /**
   * Set \ref Promise result
   *
   * \tparam Type \ref Result<T> should be constructable from this type
   * \param value value
   */
  template <typename Type>
  void Set(Type&& value) &&;

  /**
   * Set \ref Promise<void> result
   */
  void Set() &&;

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
