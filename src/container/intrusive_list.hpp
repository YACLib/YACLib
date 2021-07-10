#pragma once

#include <yaclib/container/intrusive_node.hpp>

#include <cstddef>

namespace yaclib::container::intrusive {

template <typename T>
class List final {
 public:
  List() = default;

  List(List&&) = delete;
  List& operator=(List&&) = delete;
  List(const List&) = delete;
  List& operator=(const List&) = delete;

  bool IsEmpty() const noexcept {
    return !_head.IsLinked();
  }

  void PushBack(detail::Node<T>* node) noexcept {
    node->Link(_head._prev, &_head);
  }

  void PushFront(detail::Node<T>* node) noexcept {
    node->Link(&_head, _head._next);
  }

  T* PopBack() noexcept;

  T* PopFront() noexcept;

 private:
  detail::Node<T> _head;  // sentinel node
};

}  // namespace yaclib::container::intrusive
