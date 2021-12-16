#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {
namespace detail {

template <typename Value>
using FutureCorePtr = util::Ptr<ResultCore<Value>>;

}  // namespace detail

/**
 * Provides a mechanism to access the result of async operations
 *
 * Future and \ref Promise are like a Single Producer/Single Consumer one-shot one-element channel.
 * Use the \ref Promise to fulfill the \ref Future.
 */
template <typename T>
class Future final {
 public:
  static_assert(!std::is_reference_v<T>,
                "Future cannot be instantiated with reference, you can use std::reference_wrapper or pointer");
  static_assert(!std::is_volatile_v<T> && !std::is_const_v<T>,
                "Future cannot be instantiated with cv qualifiers, because it's unnecessary");
  static_assert(!util::IsFutureV<T>, "Future cannot be instantiated with Future");
  static_assert(!util::IsResultV<T>, "Future cannot be instantiated with Result");
  static_assert(!std::is_same_v<T, std::error_code>, "Future cannot be instantiated with std::error_code");
  static_assert(!std::is_same_v<T, std::exception_ptr>, "Future cannot be instantiated with std::exception_ptr");

  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  /**
   * The default constructor creates not a \ref Valid Future
   *
   * Needed only for usability, e.g. instead of std::optional<util::Future<T>> in containers.
   */
  Future() = default;
  Future(Future&& other) noexcept = default;
  Future& operator=(Future&& other) noexcept = default;

  /**
   * If Future is \ref Valid then call \ref Stop
   */
  ~Future();

  /**
   * Check if this \ref Future has \ref Promise
   *
   * \return false if this \ref Future is default-constructed or moved to, otherwise true
   */
  [[nodiscard]] bool Valid() const& noexcept;

  /**
   * Check that \ref Result that corresponds to this \ref Future is computed
   *
   * \return false if the \Result of this \ref Future is not computed yet, otherwise true
   */
  [[nodiscard]] bool Ready() const& noexcept;

  void Get() & = delete;
  void Get() const&& = delete;

  /**
   * Return copy of \ref Result from \ref Future
   *
   * If \ref Ready is false return an empty \ref Result. This method is thread-safe and can be called multiple times.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \return \ref Result stored in the shared state
   */
  [[nodiscard]] const util::Result<T>* Get() const& noexcept;

  /**
   * Wait until \def Ready is true and move \ref Result from Future
   *
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \return The \ref Result that Future received
   */
  [[nodiscard]] util::Result<T> Get() && noexcept;

  /**
   * Stop pipeline before current step, if possible.
   */
  void Stop() &&;

  /**
   * Disable calling \ref Stop in destructor
   */
  void Detach() &&;

  /**
   * Specify executor for continuation.
   */
  Future& Via(IExecutorPtr executor) &;
  Future&& Via(IExecutorPtr executor) &&;

  /**
   * Attach the continuation functor to *this
   *
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param f A continuation to be attached
   * \return New \ref Future object associated with the functor result
   */
  template <typename Functor>
  [[nodiscard]] auto Then(Functor&& f) &&;

  /**
   * Attach the continuation functor to *this
   *
   * The functor will be executed via \ref Inline executor.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param f A continuation to be attached
   * \return New \ref Future object associated with the functor result
   */
  template <typename Functor>
  [[nodiscard]] auto ThenInline(Functor&& f) &&;

  /**
   * Attach the continuation functor to *this
   *
   * The functor will be executed via the specified executor.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param e Executor which will \ref Execute the continuation
   * \param f A continuation to be attached
   * \return New \ref Future object associated with the functor result
   */
  template <typename Functor>
  [[nodiscard]] auto Then(IExecutorPtr e, Functor&& f) &&;

  /**
   * Attach the final continuation functor to *this
   *
   * \note Functor must return void type.
   * \param f A continuation to be attached
   */
  template <typename Functor>
  void Subscribe(Functor&& functor) &&;

  /**
   * Attach the final continuation functor to *this
   *
   * The functor will be executed via \ref Inline executor.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param f A continuation to be attached
   */
  template <typename Functor>
  void SubscribeInline(Functor&& f) &&;

  /**
   * Attach the final continuation functor to *this
   *
   * The functor will be executed via the specified executor.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param e Executor which will \ref Execute the continuation
   * \param f A continuation to be attached
   */
  template <typename Functor>
  void Subscribe(IExecutorPtr e, Functor&& f) &&;

  /// DETAIL
  explicit Future(detail::FutureCorePtr<T> core);
  [[nodiscard]] const detail::FutureCorePtr<T>& GetCore() const;
  /// DETAIL

 private:
  detail::FutureCorePtr<T> _core;
};

extern template class Future<void>;

Future<void> MakeFuture();

template <typename V, typename T = std::remove_reference_t<V>>
Future<T> MakeFuture(V&& value) {
  return Future<T>{util::MakeIntrusive<detail::ResultCore<T>>(std::forward<V>(value))};
}

template <typename T, typename V>
std::enable_if_t<!std::is_same_v<T, std::remove_reference_t<V>>, Future<T>> MakeFuture(V&& value) {
  return Future<T>{util::MakeIntrusive<detail::ResultCore<T>>(std::forward<V>(value))};
}

}  // namespace yaclib

#ifndef YACLIB_ASYNC_DECL

#define YACLIB_ASYNC_DECL
#include <yaclib/async/promise.hpp>
#undef YACLIB_ASYNC_DECL

#define YACLIB_ASYNC_IMPL
#include <yaclib/async/detail/future_impl.hpp>
#include <yaclib/async/detail/promise_impl.hpp>
#undef YACLIB_ASYNC_IMPL

#endif
