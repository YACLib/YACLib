#include <yaclib/log.hpp>

#include <cstdint>
#include <yaclib_std/atomic>
#include <yaclib_std/condition_variable>
#include <yaclib_std/mutex>
#include <yaclib_std/thread>

namespace yaclib {

struct Inner {
 public:
  void Park() {
    for (size_t i = 0; i < 3; ++i) {
      auto state = State::Notified;
      if (_state.compare_exchange_strong(state, State::Empty)) {
        return;
      }
      // yaclib_std::this_thread::yield();
    }
    // TODO(kononovk): driver
    ParkCondvar();  // Going to sleep
  }

  void Unpark() {
    auto state = _state.exchange(State::Notified);
    if (state == State::ParkedCondvar) {
      UnparkCondvar();
    }
    // TODO(kononovk) Driver
  }

  void Shutdown() {
    _cv.notify_all();
  }

 private:
  void ParkCondvar() {
    std::unique_lock lock{_mutex};
    auto state = State::Empty;
    [[maybe_unused]] bool cas_ok = _state.compare_exchange_strong(state, State::ParkedCondvar);
    if (state == State::Notified) {
      auto old = _state.exchange(State::Empty);
      YACLIB_ASSERT(old == State::Notified);  // park state changed unexpectedly
      return;
    }
    YACLIB_ASSERT(cas_ok);

    while (true) {
      _cv.wait(lock);
      state = State::Notified;
      if (_state.compare_exchange_strong(state, State::Empty)) {
        return;  // got a notification
      }
      // go back to sleep
    }
  }

  void UnparkCondvar() {
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
  yaclib_std::atomic<State> _state;
};

}  // namespace yaclib
