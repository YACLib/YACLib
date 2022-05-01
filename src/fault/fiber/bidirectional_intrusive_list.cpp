#include <yaclib/fault/detail/fiber/bidirectional_intrusive_list.hpp>

#include <utility>

namespace yaclib::detail {

void BiList::PushBack(BiNode* node) noexcept {
  _size++;
  node->next = _tail->next;
  _tail->next = node;
  node->prev = _tail;
  _head.prev = node;
  _tail = node;
}

bool BiList::Empty() const noexcept {
  return &_head == _head.prev;
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
  if (node == _tail) {
    _tail = prev;
  }
  node->next = nullptr;
  node->prev = nullptr;
  return true;
}

BiList::BiList(BiList&& other) noexcept {
  if (this == &other || other.Empty()) {
    return;
  }
  _head.next = std::exchange(other._head.next, &other._head);
  _head.prev = std::exchange(other._head.prev, &other._head);
  _tail = std::exchange(other._tail, &other._head);
}

BiNode* BiList::PopBack() {
  _size--;
  auto* elem = _tail;
  Erase(_tail);
  return elem;
}

std::size_t BiList::GetSize() const noexcept {
  return _size;
}

BiNode* BiList::GetNth(std::size_t ind) const noexcept {
  YACLIB_DEBUG(ind >= _size, "ind for BiList::GetNth is out of bounds");
  std::size_t indd = 0;
  BiNode* node = _head.next;
  while (indd < ind) {
    node = node->next;
    indd++;
  }
  return node;
}

BiList& BiList::operator=(BiList&& other) noexcept {
  if (this == &other || other.Empty()) {
    return *this;
  }
  _head.next = std::exchange(other._head.next, &other._head);
  _head.prev = std::exchange(other._head.prev, &other._head);
  _tail = std::exchange(other._tail, &other._head);
  return *this;
}
}  // namespace yaclib::detail
