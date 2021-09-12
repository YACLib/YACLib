#pragma once
#include <yaclib/async/detail/core.hpp>

#include <utility>

namespace yaclib::async {

template <typename T>
class Future;

template <typename T>
class Promise {
  static_assert(!std::is_reference_v<T>,
                "Promise cannot be instantiated with reference, "
                "you can use std::reference_wrapper or pointer");
  static_assert(!std::is_volatile_v<T> && !std::is_const_v<T>,
                "Promise cannot be instantiated with cv qualifiers, because it's unnecessary");
  static_assert(!util::IsResultV<T>, "Promise cannot be instantiated with Result");
  static_assert(!util::IsFutureV<T>, "Promise cannot be instantiated with Future");
  static_assert(!std::is_same_v<T, std::error_code>, "Promise cannot be instantiated with std::error_code");
  static_assert(!std::is_same_v<T, std::exception_ptr>, "Promise cannot be instantiated with std::exception_ptr");

 public:
  Promise(const Promise& other) = delete;
  Promise& operator=(const Promise& other) = delete;

  Promise();
  Promise(Promise&& other) noexcept = default;
  Promise& operator=(Promise&& other) noexcept = default;

  /**
   * \brief Create and return \ref Future associated with this promise
   *
   * \note You cat extract \ref Future only once
   * \return New \ref Future object associated with *this
   */
  Future<T> MakeFuture();

  /**
   * \brief Set \ref Promise result.
   *
   * \tparam Type \ref Result<T> should be constructible from this type.
   * @param value
   */
  template <typename Type>
  void Set(Type&& value) &&;

  /**
   * \brief Set \ref Promise result.
   */
  void Set() &&;

 private:
  detail::PromiseCorePtr<T> _core;
  bool _future_extracted{false};  // TODO should be in _core bit
};

/**
 * \brief Describes channel with future and promise.
 */
template <typename T>
struct Contract {
  Future<T> future;
  Promise<T> promise;
};

/**
 * \brief Creates related future and promise
 *
 * \return \see Contract object with new future and promise
 */
template <typename T>
Contract<T> MakeContract();

}  // namespace yaclib::async
