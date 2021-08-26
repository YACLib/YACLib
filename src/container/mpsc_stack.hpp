#pragma once

#include <yaclib/config.hpp>
#include <yaclib/container/intrusive_node.hpp>

#include <atomic>

namespace yaclib::container::intrusive {

/** Lock free multi-producer/single-consumer stack */
class MPSCStack final {
 public:
  void Put(detail::Node* node) {
    node->_next = head_.load(std::memory_order_relaxed);
    while (!head_.compare_exchange_weak(node->_next, node,
                                        std::memory_order_release,
                                        std::memory_order_relaxed)) {
    }
  }

  detail::Node* TakeAllFIFO() {
    return Reverse(TakeAllLIFO());
  }

  detail::Node* TakeAllLIFO() {
    return head_.exchange(nullptr, std::memory_order_acquire);
  }

 private:
  static detail::Node* Reverse(detail::Node* node) {
    detail::Node* prev{};
    while (node != nullptr) {
      auto next{node->_next};
      node->_next = prev;
      prev = node;
      node = next;
    }
    return prev;
  }

  alignas(kCacheLineSize) std::atomic<detail::Node*> head_{nullptr};
};

}  // namespace yaclib::container::intrusive
