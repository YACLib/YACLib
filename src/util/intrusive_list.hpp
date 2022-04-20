#pragma once

#include <yaclib/util/detail/node.hpp>

namespace yaclib::detail {

class List final {
 public:
  List& operator=(const List&) = delete;
  List& operator=(List&&) = delete;
  List(const List&) = delete;

  List() noexcept = default;
  List(List&&) noexcept;

  void PushFront(Node& node) noexcept;
  void PushBack(Node& node) noexcept;

  [[nodiscard]] bool Empty() const noexcept;
  [[nodiscard]] Node& PopFront() noexcept;

 private:
  Node _head;
  Node* _tail = &_head;  // need for PushBack
};

}  // namespace yaclib::detail
