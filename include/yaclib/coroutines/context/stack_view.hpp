#pragma once
#include <cstddef>

namespace yaclib {

class StackView {
 public:
  StackView(char* start, size_t size) : _sp(start), _size(size) {
  }

  StackView() : StackView(nullptr, 0) {
  }

  [[nodiscard]] char* Data() const noexcept {
    return Begin();
  }

  [[nodiscard]] size_t Size() const noexcept {
    return _size;
  }

  [[nodiscard]] char* Begin() const noexcept {
    return _sp;
  }

  [[nodiscard]] char* End() const noexcept {
    return _sp + _size;
  }

  [[nodiscard]] char* Back() const noexcept {
    return End() - 1;
  }

 private:
  char* _sp;
  size_t _size;
};

}  // namespace yaclib
