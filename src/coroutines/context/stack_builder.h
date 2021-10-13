#pragma once

#include <cstdint>
#include <cstddef>

class ArgumentsListBuilder {
 public:
  explicit ArgumentsListBuilder(void* top) : _top((uintptr_t*)top) {
  }

  void Push(void* arg) {
    ++_top;
    *_top = (uintptr_t)arg;
  }

 private:
  uintptr_t* _top;
};

class StackBuilder {
  using Self = StackBuilder;

  using Word = std::uintptr_t;
  static const size_t kWordSize = sizeof(Word);

 public:
  explicit StackBuilder(char* bottom) : _top(bottom) {
  }

  void AlignNextPush(size_t alignment) {
    size_t shift = (size_t)(_top - kWordSize) % alignment;
    _top -= shift;
  }

  [[nodiscard]] void* Top() const {
    return _top;
  }

  Self& Push(Word value) {
    _top -= kWordSize;
    *(Word*)_top = value;
    return *this;
  }

  Self& Allocate(size_t bytes) {
    _top -= bytes;
    return *this;
  }

 private:
  char* _top;
};
