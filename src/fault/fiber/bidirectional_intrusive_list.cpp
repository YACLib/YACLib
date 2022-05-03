#include <yaclib/fault/detail/fiber/bidirectional_intrusive_list.hpp>

#include <utility>

namespace yaclib::detail::fiber {

void BiList::PushBack(BiNode* node) noexcept {
  if (node->prev != nullptr && node->prev != node) {
    _size++;
    this->Erase(node);
  }
  YACLIB_DEBUG(node->prev != nullptr && node->prev != node, "pushed and not popped");
  _size++;
  node->next = &_head;
  _head.prev->next = node;
  node->prev = _head.prev;
  _head.prev = node;
}

bool BiList::Empty() const noexcept {
  return _size == 0;
}

bool BiList::Erase(BiNode* node) noexcept {
  YACLIB_DEBUG(node == &_head, "trying to erase head");
  if (node->next == nullptr || node->prev == nullptr) {
    return false;
  }
  _size--;
  BiNode* prev = node->prev;
  BiNode* next = node->next;
  prev->next = next;
  next->prev = prev;
  node->next = nullptr;
  node->prev = nullptr;
  return true;
}

BiList::BiList(BiList&& other) noexcept {
  if (this == &other || other.Empty()) {
    return;
  }
  *this = std::move(other);
}

BiNode* BiList::PopBack() {
  auto* elem = _head.prev;
  Erase(_head.prev);
  return elem;
}

std::size_t BiList::GetSize() const noexcept {
  return _size;
}

BiNode* BiList::GetNth(std::size_t ind) const noexcept {
  std::size_t i = (ind + _size) % _size;
  BiNode* node;
  if (i < _size / 2) {
    std::size_t current_i = 0;
    node = _head.next;
    while (current_i < i) {
      node = node->next;
      current_i++;
    }
  } else {
    std::size_t current_i = _size - 1;
    node = _head.prev;
    while (current_i > 0) {
      node = node->prev;
      current_i--;
    }
  }
  return node;
}

BiList& BiList::operator=(BiList&& other) noexcept {
  if (this == &other || other.Empty()) {
    return *this;
  }
  _head.next = std::exchange(other._head.next, &other._head);
  _head.prev = std::exchange(other._head.prev, &other._head);
  _head.next->prev = &_head;
  _head.prev->next = &_head;
  _size = std::exchange(other._size, 0);
  return *this;
}

void BiList::PushAll(BiList& other) noexcept {
  if (this == &other || other.Empty()) {
    return;
  }
  _head.prev->next = std::exchange(other._head.next, &other._head);
  _head.prev->next->prev = _head.prev;
  _head.prev = std::exchange(other._head.prev, &other._head);
  _head.prev->next = &_head;
  _size += other._size;
  other._size = 0;
}

void BiList::DecSize() noexcept {
  --_size;
}
}  // namespace yaclib::detail::fiber
