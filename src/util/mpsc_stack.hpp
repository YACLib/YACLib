#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fault/atomic.hpp>
#include <yaclib/util/detail/node.hpp>

namespace yaclib::detail {

/**
 * Lock free multi-producer/single-consumer stack
 */
class MPSCStack final {
 public:
  void Put(Node& node);

  [[nodiscard]] Node* TakeAllLIFO();

  [[nodiscard]] Node* TakeAllFIFO();

 private:
  alignas(kCacheLineSize) yaclib_std::atomic<Node*> _head{nullptr};
};

}  // namespace yaclib::detail
