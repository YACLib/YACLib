#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

/**
 * Provides a mechanism to access the result of async operations
 *
 * Future and \ref Promise are like a Single Producer/Single Consumer one-shot one-element channel.
 * Use the \ref Promise to fulfill the \ref Future.
 */
template <typename V, typename E = StopError>
class Future final {
 public:
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>, "Future cannot be instantiated with same V and E, because it's ambiguous");

  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  Future(Future&& other) noexcept = default;
  Future& operator=(Future&& other) noexcept = default;

  /**
   * The default constructor creates not a \ref Valid Future
   *
   * Needed only for usability, e.g. instead of std::optional<Future<T>> in containers.
   */
  Future() = default;

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
   * \return false if the \ref Result of this \ref Future is not computed yet, otherwise true
   */
  [[nodiscard]] bool Ready() const& noexcept;

  void Get() & = delete;
  void Get() const&& = delete;
  void GetUnsafe() & = delete;
  void GetUnsafe() const&& = delete;

  /**
   * Return copy of \ref Result from \ref Future
   *
   * If \ref Ready is false return an empty \ref Result. This method is thread-safe and can be called multiple times.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \return \ref Result stored in the shared state
   */
  [[nodiscard]] const Result<V, E>* Get() const& noexcept;

  /**
   * Wait until \def Ready is true and move \ref Result from Future
   *
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \return The \ref Result that Future received
   */
  [[nodiscard]] Result<V, E> Get() && noexcept;

  /**
   * Return const ref of \ref Result from \ref Future
   *
   * Assume Ready is true. This method is NOT thread-safe and can be called multiple
   * times. \note The behavior is undefined if \ref Valid is false before the call to this function. \return \ref Result
   * stored in the shared state
   */
  [[nodiscard]] const Result<V, E>& GetUnsafe() const& noexcept;

  /**
   * Assume \def Ready is true and move \ref Result from Future
   *
   * \note The behavior is undefined if \ref Valid or Ready is false before the call to this function.
   * \return The \ref Result that Future received
   */
  [[nodiscard]] Result<V, E> GetUnsafe() && noexcept;

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
  void Subscribe(Functor&& f) &&;

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
  Future(detail::ResultCorePtr<V, E> core) noexcept;
  [[nodiscard]] detail::ResultCorePtr<V, E>& GetCore() noexcept;
  /// DETAIL

 private:
  detail::ResultCorePtr<V, E> _core;
};

extern template class Future<void, StopError>;

template <typename E = StopError>
Future<void, E> MakeFuture() {
  return {MakeIntrusive<detail::ResultCore<void, E>>(Unit{})};
}

template <typename T, typename E = StopError, typename V = std::remove_reference_t<T>>
Future<V, E> MakeFuture(T&& value) {
  return {MakeIntrusive<detail::ResultCore<V, E>>(std::forward<T>(value))};
}

template <typename V, typename E = StopError, typename T>
std::enable_if_t<!std::is_same_v<V, std::remove_reference_t<T>>, Future<V, E>> MakeFuture(T&& value) {
  return {MakeIntrusive<detail::ResultCore<V, E>>(std::forward<T>(value))};
}

}  // namespace yaclib

#include <yaclib/async/detail/future_impl.hpp>
