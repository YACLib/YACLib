#pragma once
#include <yaclib/log.hpp>

#include <cstdint>

namespace yaclib::detail::fiber {

struct BiNode {
  BiNode* prev = this;
  BiNode* next = this;
};

class BiList final {
 public:
  BiList& operator=(const BiList&) = delete;
  BiList& operator=(BiList&&) noexcept;
  BiList(const BiList&) = delete;

  BiList() noexcept = default;
  BiList(BiList&&) noexcept;

  void PushBack(BiNode* node) noexcept;

  void PushAll(BiList& other) noexcept;

  BiNode* PopBack();

  [[nodiscard]] bool Empty() const noexcept;

  bool Erase(BiNode* node) noexcept;

  [[nodiscard]] std::size_t GetSize() const noexcept;

  void DecSize() noexcept;

  [[nodiscard]] BiNode* GetNth(std::size_t ind) const noexcept;

 private:
  BiNode _head;
  std::size_t _size{0};
};

}  // namespace yaclib::detail::fiber
