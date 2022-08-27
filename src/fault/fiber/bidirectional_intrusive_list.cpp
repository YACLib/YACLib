#include <yaclib/fault/detail/fiber/bidirectional_intrusive_list.hpp>

#include <utility>

namespace yaclib::detail::fiber {

void BiList::PushBack(Node* node) noexcept {
  YACLIB_DEBUG(node->prev != nullptr && node->prev != node, "Pushed but not popped");
  node->next = &_head;
  _head.prev->next = node;
  node->prev = _head.prev;
  _head.prev = node;
}

bool BiList::Empty() const noexcept {
  return _head.next == &_head;
}

BiList::BiList(BiList&& other) noexcept {
  if (this == &other || other.Empty()) {
    return;
  }
  *this = std::move(other);
}

Node* BiList::PopBack() noexcept {
  auto* elem = _head.prev;
  elem->Erase();
  return elem;
}

Node* BiList::GetElement(std::size_t ind, bool reversed) const noexcept {
  std::size_t i = 0;
  Node* node{};
  if (reversed) {
    node = _head.prev;
  } else {
    node = _head.next;
  }
  while (i != ind) {
    if (node == &_head) {
      break;
    }
    ++i;
    if (reversed) {
      node = node->prev;
    } else {
      node = node->next;
    }
  }
  if (i == ind && node != &_head) {
    return node;
  }
  auto size = i;
  if (size == 0) {
    return nullptr;
  }
  i = reversed ? (size - ind % size) % size : ind % size;
  if (i < size / 2) {
    std::size_t current_i = 0;
    node = _head.next;
    while (current_i < i) {
      node = node->next;
      current_i++;
    }
  } else {
    std::size_t current_i = size - 1;
    node = _head.prev;
    while (current_i > i) {
      node = node->prev;
      current_i--;
    }
  }
  return node;
}

BiList& BiList::operator=(BiList&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  if (other.Empty()) {
    _head.next = &_head;
    _head.prev = &_head;
    return *this;
  }
  _head.next = std::exchange(other._head.next, &other._head);
  _head.prev = std::exchange(other._head.prev, &other._head);
  _head.next->prev = &_head;
  _head.prev->next = &_head;
  return *this;
}

void BiList::PushAll(BiList&& other) noexcept {
  if (this == &other || other.Empty()) {
    return;
  }
  _head.prev->next = std::exchange(other._head.next, &other._head);
  _head.prev->next->prev = _head.prev;
  _head.prev = std::exchange(other._head.prev, &other._head);
  _head.prev->next = &_head;
}

bool Node::Erase() {
  if (this->next == nullptr || this->prev == nullptr) {
    return false;
  }
  Node* prev_node = this->prev;
  Node* next_node = this->next;
  prev_node->next = next_node;
  next_node->prev = prev_node;
  this->next = nullptr;
  this->prev = nullptr;
  return true;
}

}  // namespace yaclib::detail::fiber
