#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::async {

/**
 * \brief Provides a mechanism to access the result of async operations.
 *
 * Future and \ref Promise are like a Single Producer/Single Consumer one-shot one-element channel.
 * Use the  \ref Promise to fulfill the Future.
 */
template <typename T>
class Future final {
 public:
  static_assert(!std::is_reference_v<T>,
                "Future cannot be instantiated with reference, "
                "you can use std::reference_wrapper or pointer");
  static_assert(!std::is_volatile_v<T> && !std::is_const_v<T>,
                "Future cannot be instantiated with cv qualifiers, because it's unnecessary");
  static_assert(!util::IsFutureV<T>, "Future cannot be instantiated with Future");
  static_assert(!util::IsResultV<T>, "Future cannot be instantiated with Result");
  static_assert(!std::is_same_v<T, std::error_code>, "Future cannot be instantiated with std::error_code");
  static_assert(!std::is_same_v<T, std::exception_ptr>, "Future cannot be instantiated with std::exception_ptr");

  Future(Future&& other) noexcept = default;
  Future& operator=(Future&& other) noexcept = default;

  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  /**
   * \brief The default constructor creates not a \ref Valid Future
   *
   * Needed only for usability, e.g. instead of std::optional<util::Future<T>> in containers.
   */
  Future() = default;

  /**
   * \brief if Future is \ref Valid then call \ref Stop.
   */
  ~Future();

  /**
   * \brief Check if this \ref Future has \ref Promise.
   *
   * \return false if this \ref Future is default-constructed or moved to, otherwise true.
   */
  [[nodiscard]] bool Valid() const& noexcept;

  /**
   * \brief Check that \ref Result that corresponds to this \ref Future is computed.
   *
   * \return false if the \Result of this \ref Future is not computed yet, otherwise true.
   */
  [[nodiscard]] bool Ready() const& noexcept;

  /**
   * \brief Wait until \ref Ready becomes true.
   */
  void Wait() &;

  /**
   * \brief Wait until the specified timeout duration has elapsed or \ref Ready becomes true.
   *
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   *       This function may block for longer than timeout_duration
   *       due to scheduling or resource contention delays.
   * \param timeout_duration maximum duration to block for.
   * \return The result of \ref Ready upon exiting.
   */
  template <typename Rep, typename Period>
  bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration) &;

  /**
   * \brief Wait until specified time has been reached or \ref Ready becomes true.
   *
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   *       This function may block for longer than until after timeout_time has been reached
   *       due to scheduling or resource contention delays.
   * \param timeout_time maximum time point to block until.
   * \return The result of \ref Ready upon exiting.
   */
  template <typename Clock, typename Duration>
  bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time) &;

  /**
   * \brief Return copy of \ref Result from \ref Future.
   *        If \ref Ready is false return an empty \ref Result.
   *
   * This method is thread-safe and can be called multiple times.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \return \ref Result stored in the shared state
   */
  [[nodiscard]] util::Result<T> Get() const&;

  /**
   * \brief Wait until \def Ready is true and move \ref Result from Future.
   *
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \return The \ref Result that Future received.
   */
  [[nodiscard]] util::Result<T> Get() &&;

  util::Result<T> Get() const&& = delete;
  util::Result<T> Get() & = delete;

  /**
   * \brief Stop pipeline before current step, if possible. Used in destructor.
   */
  void Stop() &&;

  /**
   * \brief Disable calling \ref Stop in destructor.
   */
  void Detach() &&;

  /**
   * \brief Attach the continuation functor to *this.
   *
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param functor A continuation to be attached.
   * \return New \ref Future object associated with the functor result.
   */
  template <typename Functor>
  [[nodiscard]] auto Then(Functor&& functor) &&;

  /**
   * \brief Attach the continuation functor to *this. The functor will be executed via the specified executor.
   *
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param executor Executor which will \ref Execute the continuation.
   * \param functor A continuation to be attached.
   * \return New \ref Future object associated with the functor result.
   */
  template <typename Functor>
  [[nodiscard]] auto Then(executor::IExecutorPtr executor, Functor&& functor) &&;

  /**
   * \brief Attach the final continuation functor to *this.
   * \note Functor must return void type.
   *
   * \param functor A continuation to be attached.
   */
  template <typename Functor>
  void Subscribe(Functor&& functor) &&;

  /**
   * \brief Attach the final functor to *this. The functor will be executed via the specified executor.
   *
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param executor Executor which will \ref Execute the continuation.
   * \param functor A continuation to be attached.
   */
  template <typename Functor>
  void Subscribe(executor::IExecutorPtr executor, Functor&& functor) &&;

  /**
   * \brief Detail constructor.
   */
  explicit Future(detail::FutureCorePtr<T> core);

  /**
   * \brief Detail method.
   */
  Future& Via(executor::IExecutorPtr executor) &;

 private:
  detail::FutureCorePtr<T> _core;
};

}  // namespace yaclib::async
