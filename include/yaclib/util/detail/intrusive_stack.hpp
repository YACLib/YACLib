#pragma once

#include <yaclib/log.hpp>
#include <yaclib/util/detail/node.hpp>

#include <utility>

namespace yaclib::detail {

class Stack final {
 public:
  Stack& operator=(const Stack&) = delete;
  Stack& operator=(Stack&&) = delete;
  Stack(const Stack&) = delete;

  Stack() noexcept = default;
  Stack(Stack&& other) noexcept : _head{std::exchange(other._head, nullptr)} {
  }

  void PushFront(Node& node) noexcept {
    node.next = _head;
    _head = &node;
  }

  // Mostly convinient helper to use lifo order instead of fifo order with List
  void PushBack(Node& node) noexcept {
    return PushFront(node);
  }

  [[nodiscard]] bool Empty() const noexcept {
    return _head == nullptr;
  }

  [[nodiscard]] Node& PopFront() noexcept {
    YACLIB_ASSERT(!Empty());
    return *std::exchange(_head, _head->next);
  }

 private:
  Node* _head = nullptr;
};

}  // namespace yaclib::detail
