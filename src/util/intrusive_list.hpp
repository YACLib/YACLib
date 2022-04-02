#pragma once

#include <yaclib/config.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/executor/thread_factory.hpp>
#include <yaclib/util/detail/node.hpp>

#include <cstddef>

namespace yaclib::detail {

template <typename T>
class List final {
 public:
  List& operator=(const List&) = delete;
  List& operator=(List&&) = delete;
  List(const List&) = delete;

  List() noexcept = default;
  List(List&&) noexcept;

  void PushFront(Node& node) noexcept;
  void PushBack(Node& node) noexcept;

  [[nodiscard]] bool Empty() const noexcept;
  [[nodiscard]] T& PopFront() noexcept;

 private:
  Node _head;
  Node* _tail = &_head;  // need for PushBack
};

extern template class List<ITask>;
extern template class List<IThread>;

}  // namespace yaclib::detail
