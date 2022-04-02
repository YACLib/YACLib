#include <util/mpsc_stack.hpp>

#include <yaclib/config.hpp>
#include <yaclib/util/detail/node.hpp>

namespace yaclib::detail {
namespace {

Node* Reverse(Node* node) {
  Node* prev = nullptr;
  while (node != nullptr) {
    auto* next = node->next;
    node->next = prev;
    prev = node;
    node = next;
  }
  return prev;
}

}  // namespace

void MPSCStack::Put(Node& node) {
  node.next = _head.load(std::memory_order_relaxed);
  while (!_head.compare_exchange_weak(node.next, &node, std::memory_order_release, std::memory_order_relaxed)) {
  }
}

Node* MPSCStack::TakeAllLIFO() {
  return _head.exchange(nullptr, std::memory_order_acquire);
}

Node* MPSCStack::TakeAllFIFO() {
  return Reverse(TakeAllLIFO());
}

}  // namespace yaclib::detail
