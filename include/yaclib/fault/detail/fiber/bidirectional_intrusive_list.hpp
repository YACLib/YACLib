#pragma once

#include <yaclib/log.hpp>

#include <cstdint>

namespace yaclib::detail::fiber {

struct Node {
  Node* prev{this};
  Node* next{this};

  bool Erase();
};

class BiList final {
 public:
  BiList& operator=(const BiList&) = delete;
  BiList& operator=(BiList&&) noexcept;
  BiList(const BiList&) = delete;

  BiList() noexcept = default;
  BiList(BiList&&) noexcept;

  void PushBack(Node* node) noexcept;

  void PushAll(BiList&& other) noexcept;

  Node* PopBack() noexcept;

  [[nodiscard]] bool Empty() const noexcept;

  [[nodiscard]] Node* GetElement(std::size_t ind, bool reversed) const noexcept;

 private:
  Node _head;
};

}  // namespace yaclib::detail::fiber
