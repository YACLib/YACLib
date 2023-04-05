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
 * Bounded MPMC channel
 * */
template <typename T>
class BoundedChannel {
  class PushAwaiter : public detail::Node {
   public:
    PushAwaiter() = default;

    template <typename... Args>
    explicit PushAwaiter(std::unique_lock<std::mutex>& lock, BoundedChannel& channel, Args&&... args)
      : _channel{&channel}, _value{std::forward<Args>(args)...} {
      lock.release();
    }

    bool await_ready() noexcept {
      return _channel == nullptr;
    }

    template <typename Promise>
    void await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
      _core = &handle.promise();
      _channel->_senders.PushFront(*this);
      _channel->_mutex.unlock();
    }

    constexpr void await_resume() noexcept {
    }

    void Resume(T& value) {
      value = std::move(_value);
      _core->_executor->Submit(*_core);
    }

    void Resume(std::unique_lock<std::mutex>& lock) {
      _channel->_queue.push(std::move(_value));
      lock.unlock();
      _core->_executor->Submit(*_core);
    }

   private:
    BoundedChannel* _channel{nullptr};
    detail::BaseCore* _core{nullptr};
    T _value;
  };

  class PopAwaiter : public detail::Node {
   public:
    explicit PopAwaiter(BoundedChannel& channel) : _channel{channel} {
    }

    bool await_ready() {
      std::unique_lock lock{_channel._mutex};
      if (_channel._queue.empty()) {
        if (_channel._senders.Empty()) {
          lock.release();
          return false;
        }
        auto& sender = _channel._senders.PopFront();
        lock.unlock();
        static_cast<PushAwaiter&>(sender).Resume(_value);
      } else {
        _value = std::move(_channel._queue.front());
        _channel._queue.pop();
        if (!_channel._senders.Empty()) {
          auto& sender = _channel._senders.PopFront();
          static_cast<PushAwaiter&>(sender).Resume(lock);
        }
      }
      return true;
    }

    template <typename Promise>
    void await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
      _core = &handle.promise();
      _channel._receivers.PushFront(*this);
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
    BoundedChannel& _channel;
    detail::BaseCore* _core{};
    T _value;
  };

 public:
  BoundedChannel(size_t size) : _size{size} {
  }

  template <typename... Args>
  PushAwaiter Push(Args&&... args) {
    std::unique_lock lock{_mutex};
    if (_receivers.Empty()) {
      if (_queue.size() == _size) {
        return PushAwaiter{lock, *this, std::forward<Args>(args)...};
      }
      _queue.emplace(std::forward<Args>(args)...);
    } else {
      YACLIB_ASSERT(_queue.empty());
      auto& receiver = _receivers.PopFront();
      lock.unlock();
      static_cast<PopAwaiter&>(receiver).Produce(std::forward<Args>(args)...);
    }
    return {};
  }

  PopAwaiter Pop() {
    return PopAwaiter{*this};
  }

 private:
  std::mutex _mutex;
  std::queue<T> _queue;
  std::size_t _size;
  detail::List _senders;
  detail::List _receivers;
};

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
