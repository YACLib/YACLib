#pragma once

#include <yaclib/async/core.hpp>
#include <yaclib/executor/executor.hpp>

#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

namespace yaclib::async {
using namespace std::chrono_literals;

namespace detail {

template <typename Functor, typename Arg>
struct invoke {
  using type = std::invoke_result_t<Functor, Arg>;
};

template <typename Functor>
struct invoke<Functor, void> {
  using type = std::invoke_result_t<Functor>;
};

template <typename Functor, typename Arg>
using invoke_t = typename invoke<Functor, Arg>::type;

}  // namespace detail

template <typename T>
class Future final {
 public:
  Future(FutureCorePtr<T> state) : _state(std::move(state)) {
  }
  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  Future(Future&& other) noexcept : _state{std::move(other._state)} {
  }

  Future& operator=(Future&& other) {
    _state.Swap(other._state);
    return *this;
  }

  template <typename Functor, typename U = detail::invoke_t<Functor, T>>
  Future<U> ThenVia(executor::IExecutorPtr executor, Functor&& functor) && {
    using CoreType = Core<U, std::decay_t<Functor>, T>;
    auto shared_state = container::intrusive::Ptr{
        new container::Counter<CoreType>{std::forward<Functor>(functor)}};
    shared_state->SetExecutor(executor);
    if constexpr (!std::is_void_v<T>) {
      shared_state->SetPrev(_state);
    }
    _state->SetCallback(*shared_state);
    return Future<U>{shared_state};
  }

  template <typename Functor, typename U = detail::invoke_t<Functor, T>>
  Future<U> Then(Functor&& functor) && {
    return std::move(*this).ThenVia(_state->GetExecutor(),
                                    std::forward<Functor>(functor));
  }

  ~Future() {
    //_state->Cancel();
  }

  T Get() && {
    std::mutex m;
    bool is_ready = false;
    std::condition_variable cv;
    if constexpr (std::is_void_v<T>) {
      auto future = std::move(*this).Then([&m, &is_ready, &cv]() {
        std::unique_lock guard{m};
        is_ready = true;
        cv.notify_all();
        return 0;
      });

      std::unique_lock guard{m};
      while (!is_ready) {
        cv.wait(guard);
      }
    } else {
      std::aligned_storage_t<sizeof(T), alignof(T)> res;
      auto future = std::move(*this).Then([&m, &is_ready, &cv, &res](T arg) {
        new (&res) T{std::move(arg)};
        std::unique_lock guard{m};
        is_ready = true;
        cv.notify_all();
      });

      std::unique_lock guard{m};
      while (!is_ready) {
        cv.wait(guard);
      }
      return std::move(*reinterpret_cast<T*>(&res));
    }
  }

  bool IsReady() const noexcept {
    return _state->HasResult();
  }

  bool IsValid() const noexcept {
    return static_cast<bool>(_state);
  }

 private:
  FutureCorePtr<T> _state;
};

}  // namespace yaclib::async