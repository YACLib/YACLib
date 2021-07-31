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
    _state = nullptr;
    return Future<U>{shared_state};
  }

  template <typename Functor, typename U = detail::invoke_t<Functor, T>>
  Future<U> Then(Functor&& functor) && {
    return std::move(*this).ThenVia(_state->GetExecutor(),
                                    std::forward<Functor>(functor));
  }

  ~Future() {
    std::move(*this).Cancel();
  }

  void Cancel() && {
    if (_state) {
      _state->Cancel();
      _state = nullptr;
      std::cout << "Cancel()" << std::endl;
    }
  }

  T Get() && {
    std::mutex m;
    bool is_ready = false;
    std::condition_variable cv;
    auto get_res = [&] {
        std::unique_lock guard{m};
        is_ready = true;
        cv.notify_all();
    };

    using CoreType = Core<void, std::decay_t<decltype(get_res)>, void>;
    auto shared_state = container::NothingCounter<CoreType>{std::move(get_res)};
    shared_state.SetExecutor(_state->GetExecutor());
    _state->SetCallback(shared_state);
    std::unique_lock guard{m};
    while (!is_ready) {
      cv.wait(guard);
    }
    if constexpr (!std::is_void_v<T>) {
      auto result = _state->GetResult();
      _state = nullptr;
      return result;
    } else {
      _state = nullptr;
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