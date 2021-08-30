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

  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  explicit Future(FutureCorePtr<T> state);
  Future(Future&& other) noexcept = default;
  Future& operator=(Future&& other) noexcept = default;

  template <typename Functor>
  auto Then(executor::IExecutorPtr executor, Functor&& functor) &&;

  template <typename Functor>
  auto Then(Functor&& functor) &&;

  template <typename Functor>
  void Subscribe(executor::IExecutorPtr executor, Functor&& functor) &&;

  template <typename Functor>
  void Subscribe(Functor&& functor) &&;

  ~Future();

  void Cancel() &&;

  util::Result<T> Get() &&;

  bool IsReady() const noexcept;

 private:
  template <typename U>
  friend class Future;

  template <typename U, typename Functor>
  Future<U> ThenResult(executor::IExecutorPtr executor, Functor&& functor);

  template <typename U, typename Functor>
  Future<U> ThenValue(executor::IExecutorPtr executor, Functor&& functor);

  template <typename Functor>
  Future<T> ThenError(executor::IExecutorPtr executor, Functor&& functor);

  template <typename Functor>
  Future<T> ThenException(executor::IExecutorPtr executor, Functor&& functor);

  template <typename U, typename Functor>
  Future<U> AsyncThenResult(executor::IExecutorPtr executor, Functor&& functor);

  template <typename U, typename Functor>
  Future<U> AsyncThenValue(executor::IExecutorPtr executor, Functor&& functor);

  template <typename Functor>
  Future<T> AsyncThenError(executor::IExecutorPtr executor, Functor&& functor);

  template <typename Functor>
  Future<T> AsyncThenException(executor::IExecutorPtr executor, Functor&& functor);

  FutureCorePtr<T> _core;
};

}  // namespace yaclib::async
