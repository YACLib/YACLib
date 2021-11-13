#pragma once

#include <yaclib/config.hpp>
#include <yaclib/util/intrusive_node.hpp>

#include <atomic>

namespace yaclib::util {

/**
 * Lock free multi-producer/single-consumer stack
 */
class MPSCStack final {
 public:
  void Put(detail::Node* node);

  [[nodiscard]] detail::Node* TakeAllLIFO();

  [[nodiscard]] detail::Node* TakeAllFIFO();

 private:
  alignas(kCacheLineSize) std::atomic<detail::Node*> _head{nullptr};
};

}  // namespace yaclib::util
