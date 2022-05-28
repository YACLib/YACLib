#pragma once

#include <yaclib/exe/job.hpp>

#include <cstdint>
namespace yaclib::detail {

class PCore : public Job {
  static constexpr std::uintptr_t kShift = 61;

 public:
  enum State : std::uintptr_t {  // clang-format off
    kEmpty      = std::uintptr_t{0} << kShift,
    kResult     = std::uintptr_t{1} << kShift,
    kCall       = std::uintptr_t{2} << kShift,
    kHereWrap   = std::uintptr_t{3} << kShift,
    kHereCall   = std::uintptr_t{4} << kShift,
    kWaitStop   = std::uintptr_t{5} << kShift,
    kWaitDrop   = std::uintptr_t{6} << kShift,
    kWaitNope   = std::uintptr_t{7} << kShift,
    kMask       = std::uintptr_t{7} << kShift,
  };  // clang-format on

  void Call() noexcept override;
  void Drop() noexcept override;

  virtual void Here(PCore& caller, State state) noexcept;
};

}  // namespace yaclib::detail
