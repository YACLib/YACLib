#pragma once

#include "yaclib/algo/detail/base_core.hpp"
#include "yaclib/fwd.hpp"

#include <yaclib/coro/coro.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/detail/intrusive_list.hpp>
#include <yaclib/util/detail/node.hpp>

#include <mutex>
#include <queue>
#include <utility>

namespace yaclib {

/*
 * Unbounded MPMC channel
 * */
template <typename T>
class Channel {
  class Awaiter : public detail::Node {
   public:
    explicit Awaiter(Channel& channel) : _channel{channel} {
    }

    bool await_ready() {
      std::unique_lock lock{_channel._mutex};
      if (_channel._queue.empty()) {
        lock.release();
        return false;
      }
      _value = std::move(_channel._queue.front());
      _channel._queue.pop();
      return true;
    }

    template <typename Promise>
    void await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
      _core = &handle.promise();
      _channel._consumers.PushFront(*this);
      _channel._mutex.unlock();
    }

    T await_resume() {
      return std::move(_value);
    }

    template <typename... Args>
    void Produce(Args&&... args) {
      _value = T{std::forward<Args>(args)...};
      _core->_executor->Submit(*_core);
    }

   private:
    detail::BaseCore* _core{};
    Channel& _channel;
    T _value;
  };

 public:
  template <typename... Args>
  void Push(Args&&... args) {
    std::unique_lock lock{_mutex};
    if (_consumers.Empty()) {
      _queue.emplace(std::forward<Args>(args)...);
    } else {
      YACLIB_ASSERT(_queue.empty());
      auto& consumer = _consumers.PopFront();
      lock.unlock();
      static_cast<Awaiter&>(consumer).Produce(std::forward<Args>(args)...);
    }
  }

  Awaiter Pop() {
    return Awaiter{*this};
  }

 private:
  std::mutex _mutex;
  std::queue<T> _queue;
  detail::List _consumers;
};

}  // namespace yaclib
