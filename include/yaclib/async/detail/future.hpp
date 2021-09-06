#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::async {

template <typename T>
class Future final {
 public:
  static_assert(!util::IsFutureV<T>, "Future cannot be instantiated with Future");
  static_assert(!util::IsResultV<T>, "Future cannot be instantiated with Result");
  static_assert(!std::is_same_v<util::DecayT<T>, std::error_code>,
                "Future cannot be instantiated with std::error_code");
  static_assert(!std::is_same_v<util::DecayT<T>, std::exception_ptr>,
                "Future cannot be instantiated with std::exception_ptr");

  Future(Future&& other) noexcept = default;
  Future& operator=(Future&& other) noexcept = default;

  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  /**
   * \brief The default constructor creates not \ref Valid Future, needed only for usability
   */
  Future() = default;

  /**
   * \brief if Future \ref Valid then call \ref Stop
   */
  ~Future();

  /**
   * \brief Checks if the this \ref Future has \ref Promise
   * \return false if this \ref Future default-constructed or moved to, otherwise true
   */
  [[nodiscard]] bool Valid() const& noexcept;

  /**
   * \brief Checks if the this \ref Future has not Empty \ref util::Result<T>
   * \return false if this \ref Future \ref util::Result<T> not computed yet, otherwise true
   */
  [[nodiscard]] bool Ready() const& noexcept;

  /**
   * \brief Blocks until \ref Ready becomes true
   */
  void Wait() &;

  /**
   *
   * \tparam Rep
   * \tparam Period
   * \param timeout_duration
   * \return
   */
  template <typename Rep, typename Period>
  bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration) &;

  /**
   *
   * \tparam Clock
   * \tparam Duration
   * \param timeout_time
   * \return
   */
  template <typename Clock, typename Duration>
  bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time) &;

  /**
   * \brief  You can use this iff you want call Get multiple times
   * \return
   */
  [[nodiscard]] util::Result<T> Get() const&;

  /**
   *
   * \return
   */
  [[nodiscard]] util::Result<T> Get() &&;

  util::Result<T> Get() const&& = delete;
  util::Result<T> Get() & = delete;

  /**
   * \brief Stop pipeline before current step if possible
   */
  void Stop() &&;

  /**
   * \brief
   */
  void Detach() &&;

  /**
   * \brief
   */
  Future& Via(executor::IExecutorPtr executor) &;

  /**
   * \brief Make new future callback for this
   * \tparam Functor
   * \param functor
   * \return
   */
  template <typename Functor>
  [[nodiscard]] auto Then(Functor&& functor) &&;

  /**
   *
   * \tparam Functor
   * \param executor
   * \param functor
   * \return
   */
  template <typename Functor>
  [[nodiscard]] auto Then(executor::IExecutorPtr executor, Functor&& functor) &&;

  /**
   * Functor must return void type
   * \tparam Functor
   * \param functor
   */
  template <typename Functor>
  void Subscribe(Functor&& functor) &&;

  /**
   *
   * \tparam Functor
   * \param executor
   * \param functor
   */
  template <typename Functor>
  void Subscribe(executor::IExecutorPtr executor, Functor&& functor) &&;

  /**
   * \brief Detail constructor
   * @param core
   */
  explicit Future(detail::FutureCorePtr<T> core);

 private:
  detail::FutureCorePtr<T> _core;
};

}  // namespace yaclib::async
