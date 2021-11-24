#pragma once

#include <utility>

namespace yaclib::util::detail {

/**
 * Intrusive node class.
 */
class Node {
 public:
  void Link(Node* prev, Node* next) noexcept {
    _prev = prev;
    _next = next;
    prev->_next = this;
    next->_prev = this;
  }

  /**
   * Is node linked in circular list?
   *
   * \return true if node is linked in a circular list, false otherwise
   */
  bool IsLinked() const noexcept {
    return _next != this;
  }

  /**
   * Unlink this node from current list
   */
  void Unlink() noexcept {
    _prev->_next = _next;
    _next->_prev = _prev;
    _prev = this;
    _next = this;
  }

  void Swap(Node& other) noexcept {
    std::swap(_prev, other._prev);
    std::swap(_next, other._next);
  }

  Node* _prev{this};
  Node* _next{this};
};

}  // namespace yaclib::util::detail
