#pragma once

#include <yaclib/util/intrusive_node.hpp>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <utility>

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
      auto* current_head = _head.load();
      auto* current_tail = _tail.load();
      auto* next = std::atomic_ref{current_head->_next}.load();
      if (current_head == _head.load()) {
        if (current_head == current_tail) {
          if (next == nullptr) {
            return nullptr;
          }
          _tail.compare_exchange_strong(current_tail, next);
        } else {
          if (_head.compare_exchange_weak(current_head, next)) {
            assert(next != nullptr);
            assert(next != &_dummy);
            return AsItem(next);
          }
        }
      }
    }
  }

  void PushBack(detail::Node* node) noexcept {
    node->_next = nullptr;
    node->_prev = nullptr;

    while (true) {
      auto* current_tail = _tail.load();
      auto* next_tail = std::atomic_ref{current_tail->_next}.load();
      if (current_tail == _tail.load()) {
        if (next_tail != nullptr) {
          _tail.compare_exchange_strong(current_tail, next_tail);
        } else {
          detail::Node* tmp = nullptr;
          if (std::atomic_ref{current_tail->_next}.compare_exchange_weak(tmp, node)) {
            std::atomic_ref{current_tail->_next}.compare_exchange_strong(current_tail, node);
            return;
          }
        }
      }
    }
  }

  void UnsafeSwap(MichaelScottQueue& other) noexcept {
    auto tmp_dummy = other._dummy;
    auto* tmp_head = other._head.load();
    auto* tmp_tail = other._tail.load();

    other._dummy = _dummy;
    other._head.store(_head.load());
    other._tail.store(_tail.load());

    _dummy = tmp_dummy;
    _head.store(tmp_head);
    _tail.store(tmp_tail);
  }

 private:
  detail::Node _dummy{nullptr, nullptr};
  std::atomic<detail::Node*> _head;
  std::atomic<detail::Node*> _tail;
};

}  // namespace yaclib::util
