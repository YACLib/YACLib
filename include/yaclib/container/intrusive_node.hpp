#pragma once

namespace yaclib::container::intrusive::detail {

class Node {
 public:
  void Link(Node* prev, Node* next) noexcept {
    _prev = prev;
    _next = next;
    prev->_next = this;
    next->_prev = this;
  }

  // Is this node linked in a circular list?
  bool IsLinked() const noexcept {
    return _next != this;
  }

  // Unlink this node from current list
  void Unlink() noexcept {
    _prev->_next = _next;
    _next->_prev = _prev;
    _prev = this;
    _next = this;
  }

  Node* _prev{this};
  Node* _next{this};
};

}  // namespace yaclib::container::intrusive::detail
