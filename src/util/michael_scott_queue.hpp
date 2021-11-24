#pragma once

#include <yaclib/util/intrusive_node.hpp>

#include <atomic>
#include <cstddef>

namespace yaclib::util {

template <typename T>
class MichaelScottQueue final {
 public:
  MichaelScottQueue() : _head{&_dummy}, _tail{&_dummy} {};

  MichaelScottQueue(MichaelScottQueue&&) = delete;
  MichaelScottQueue& operator=(MichaelScottQueue&&) = delete;
  MichaelScottQueue(const MichaelScottQueue&) = delete;
  MichaelScottQueue& operator=(const MichaelScottQueue&) = delete;

  static T* AsItem(detail::Node* node) noexcept {
    return static_cast<T*>(node);
  }

  [[nodiscard]] T* PopFront() noexcept {
    while (true) {
      auto* saved_head = _head;
      auto* saved_tail = _tail;
      auto* saved_head_next = saved_head->_next;

      if (saved_head == saved_tail) {
        if (saved_head_next == saved_head) {
          return nullptr;
        }
        std::atomic_ref{_tail}.compare_exchange_strong(saved_tail, saved_tail->_next);
      } else {
        auto res = AsItem(saved_head_next);
        if (std::atomic_ref{_head}.compare_exchange_weak(saved_head, saved_head_next)) {
          return res;
        }
      }
    }
  }

  void PushBack(detail::Node* node) noexcept {
    while (true) {
      auto* saved_tail = _tail;
      auto* saved_tail_next = saved_tail->_next;

      if (std::atomic_ref{_tail->_next}.compare_exchange_weak(saved_tail_next, node)) {
        std::atomic_ref{_tail}.compare_exchange_strong(saved_tail, node);
        return;
      }
      std::atomic_ref{_tail}.compare_exchange_strong(saved_tail, saved_tail->_next);
    }
  }

 private:
  detail::Node _dummy;
  detail::Node* _head;
  detail::Node* _tail;
};

}  // namespace yaclib::util
