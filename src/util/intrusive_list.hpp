#pragma once

#include <yaclib/util/intrusive_node.hpp>

#include <cstddef>

namespace yaclib::util {

template <typename T>
class List final {
 public:
  List() = default;

  List(List&&) = delete;
  List& operator=(List&&) = delete;
  List(const List&) = delete;
  List& operator=(const List&) = delete;

  static T* AsItem(detail::Node* node) noexcept {
    return static_cast<T*>(node);
  }

  bool IsEmpty() const noexcept {
    return !_head.IsLinked();
  }

  void PushBack(detail::Node* node) noexcept {
    node->Link(_head._prev, &_head);
  }

  void PushFront(detail::Node* node) noexcept {
    node->Link(&_head, _head._next);
  }

  T* PopBack() noexcept;

  T* PopFront() noexcept;

  void Append(List& other) noexcept;

 private:
  detail::Node _head;  // sentinel node
};

}  // namespace yaclib::util
