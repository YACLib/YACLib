#pragma once

namespace yaclib::container::intrusive::detail {

/**
 * \brief Intrusive node class
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
   * \brief is node linked in circular list?
   *
   * \return true if node is linked in a circular list, false otherwise
   */
  bool IsLinked() const noexcept {
    return _next != this;
  }

  /**
   * \brief Unlink this node from current list
   */
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
