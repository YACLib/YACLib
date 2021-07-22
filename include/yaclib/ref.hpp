#pragma once

namespace yaclib {

class IRef {
 public:
  // IExecutor make this before use ITask
  // IThreadFactory make this before use IFunc
  virtual void Acquire() noexcept = 0;

  // IExecutor make this after use ITask
  // IThreadFactory make this after use IFunc
  virtual void Release() noexcept = 0;

  virtual ~IRef() = default;
};

}  // namespace yaclib
