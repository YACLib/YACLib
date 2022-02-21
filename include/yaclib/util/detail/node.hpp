#pragma once

namespace yaclib::detail {

/**
 * Node class, used in intrusive data structure
 */
struct Node {
  Node* next = nullptr;  // valid for linear, for circular should be this
};

}  // namespace yaclib::detail
