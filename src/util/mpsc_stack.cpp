#include <util/mpsc_stack.hpp>

#include <yaclib/util/detail/node.hpp>

namespace yaclib::util {
namespace {

detail::Node* Reverse(detail::Node* node) {
  detail::Node* prev = nullptr;
  while (node != nullptr) {
    auto* next{node->_next};
    node->_next = prev;
    prev = node;
    node = next;
  }
  return prev;
}

}  // namespace

void MPSCStack::Put(detail::Node* node) {
  node->_next = _head.load(std::memory_order_relaxed);
  while (!_head.compare_exchange_weak(node->_next, node, std::memory_order_release, std::memory_order_relaxed)) {
  }
}

detail::Node* MPSCStack::TakeAllLIFO() {
  return _head.exchange(nullptr, std::memory_order_acquire);
}

detail::Node* MPSCStack::TakeAllFIFO() {
  return Reverse(TakeAllLIFO());
}

}  // namespace yaclib::util
