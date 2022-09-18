#include <yaclib/log.hpp>

#include <cstdint>
#include <yaclib_std/atomic>
#include <yaclib_std/condition_variable>
#include <yaclib_std/mutex>
#include <yaclib_std/thread>

namespace yaclib {

struct Waiter {
 public:
  template <typename Func>
  bool Park(const Func& func) noexcept {
    for (size_t i = 0; i < 3; ++i) {
      auto expected = State::Notified;
      if (_state.compare_exchange_strong(expected, State::Empty)) {
        return func();
      }
    }
    return ParkCondvar(func);
  }

  void Unpark() noexcept {
    auto state = _state.exchange(State::Notified);
    if (state == State::ParkedCondvar) {
      UnparkCondvar();
    }
  }

 private:
  template <typename Func>
  [[nodiscard]] bool ParkCondvar(const Func& func) noexcept {
    std::unique_lock lock{_mutex};
    if (func()) {
      return true;
    }
    auto expected = State::Empty;
    [[maybe_unused]] bool cas_ok = _state.compare_exchange_strong(expected, State::ParkedCondvar);
    if (expected == State::Notified) {
      auto old = _state.exchange(State::Empty);
      YACLIB_ASSERT(old == State::Notified);  // park state changed unexpectedly
      return false;
    }
    YACLIB_ASSERT(cas_ok);
    while (true) {
      _cv.wait(lock);
      if (func()) {
        return true;
      }
      expected = State::Notified;
      if (_state.compare_exchange_strong(expected, State::Empty)) {
        return false;  // got a notification
      }
      // go back to sleep
    }
  }

  void UnparkCondvar() noexcept {
    _mutex.lock();
    _mutex.unlock();
    _cv.notify_one();
  }

  yaclib_std::mutex _mutex;
  yaclib_std::condition_variable _cv;
  enum class State : std::uint32_t {
    Empty = 0,
    ParkedCondvar = 1,
    Notified = 2,
  };
  yaclib_std::atomic<State> _state{State::Empty};
};

}  // namespace yaclib
