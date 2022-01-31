#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {
namespace detail {

template <typename Value>
using PromiseCorePtr = util::Ptr<ResultCore<Value>>;

}  // namespace detail

template <typename T>
class Future;

template <typename T>
class Promise final {
  static_assert(!std::is_reference_v<T>,
                "Promise cannot be instantiated with reference, you can use std::reference_wrapper or pointer");
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
  ~Promise();

  [[nodiscard]] bool Valid() const& noexcept;
  /**
   * Create and return \ref Future associated with this promise
   *
   * \note You cat extract \ref Future only once.
   * \return New \ref Future object associated with *this
   */
  [[nodiscard]] Future<T> MakeFuture();

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

  /// DETAIL
  explicit Promise(detail::PromiseCorePtr<T> core) noexcept;
  [[nodiscard]] detail::PromiseCorePtr<T>& GetCore() noexcept;
  /// DETAIL

 private:
  detail::PromiseCorePtr<T> _core;
};

extern template class Promise<void>;

/**
 * Describes channel with future and promise
 */
template <typename T>
#if !defined(__clang__) && __GNUC__ < 8
using Contract = std::pair<Future<T>, Promise<T>>;
#else
struct Contract {
  Future<T> future;
  Promise<T> promise;
};
#endif
/**
 * Creates related future and promise
 *
 * \return a \see Contract object with new future and promise
 */
template <typename T>
[[nodiscard]] Contract<T> MakeContract();

}  // namespace yaclib

#ifndef YACLIB_ASYNC_DECL

#define YACLIB_ASYNC_DECL
#include <yaclib/async/future.hpp>
#undef YACLIB_ASYNC_DECL

#define YACLIB_ASYNC_IMPL
#include <yaclib/async/detail/future_impl.hpp>
#include <yaclib/async/detail/promise_impl.hpp>
#undef YACLIB_ASYNC_IMPL

#endif
