#pragma once

namespace yaclib::detail {

/**
 * Node class, used in intrusive data structure
 */
class Node {
 public:
  void Link(Node* prev, Node* next) noexcept;

  /**
   * Is node linked with other node?
   *
   * \return true if node is linked with other node, false otherwise
   */
  [[nodiscard]] bool IsLinked() const noexcept;

  /**
   * Unlink this node from current list
   */
  void Unlink() noexcept;

  Node* _prev = this;
  Node* _next = this;
};

}  // namespace yaclib::detail
