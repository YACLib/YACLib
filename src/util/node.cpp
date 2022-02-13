#include <yaclib/util/detail/node.hpp>

namespace yaclib::detail {

void Node::Link(Node* prev, Node* next) noexcept {
  _prev = prev;
  _next = next;
  prev->_next = this;
  next->_prev = this;
}

bool Node::IsLinked() const noexcept {
  return _next != this;
}

void Node::Unlink() noexcept {
  _prev->_next = _next;
  _next->_prev = _prev;
  _prev = this;
  _next = this;
}

}  // namespace yaclib::detail
