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
    template<typename... Args>
    explicit PushAwaiter(BoundedChannel& channel, Args&&... args) : _channel{&channel}, _value{std::forward<Args>(args)...} {}

    bool await_ready() {
      return _channel != nullptr;
    }

    template<typename Promise>
    void await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
      _core = &handle.promise();
      _channel->_senders.PushFront(*this);
      _channel->_mutex.unlock();
    }

    void await_resume() {
    }

    void GetValue(T& value) {
      value = std::move(_value);
      _core->_executor->Submit(*_core);
    }
  private:

    T _value;
    detail::BaseCore* _core{nullptr};
    BoundedChannel* _channel{nullptr};
  };

  class PopAwaiter : public detail::Node {
   public:
    explicit PopAwaiter(BoundedChannel& channel) : _channel{channel} {
    }

    bool await_ready() {
      std::unique_lock lock{_channel._mutex};
      if (!_channel._senders.Empty()) {
        auto& s = _channel._senders.PopFront();
        lock.unlock();
        auto& pa = static_cast<PushAwaiter&>(s);
        pa.GetValue(_value);
        return false;
      }
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
    detail::BaseCore* _core{};
    BoundedChannel& _channel;
    T _value;
  };

 public:
  BoundedChannel(size_t size) : _size{size} {}

  template <typename... Args>
  PushAwaiter Push(Args&&... args) {
    std::unique_lock lock{_mutex};
    if (_receivers.Empty()) {
      if (_queue.size() == _size) {
        lock.release(); // TODO handle exception in T constructor
        return PushAwaiter{*this, std::forward<Args>(args)...};
      }
      _queue.emplace(std::forward<Args>(args)...);
    } else {
      YACLIB_ASSERT(_queue.empty());
      auto& consumer = _receivers.PopFront();
      lock.unlock();
      static_cast<PopAwaiter&>(consumer).Produce(std::forward<Args>(args)...);
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
