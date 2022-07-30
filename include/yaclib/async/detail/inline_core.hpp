#pragma once

#include <yaclib/executor/job.hpp>

#include <cstdint>

namespace yaclib::detail {

class InlineCore : public Job {
  static constexpr std::uint64_t kShift = 61;

 public:
  enum State : std::uint64_t {  // clang-format off
    kEmpty      = std::uint64_t{0} << kShift,
    kResult     = std::uint64_t{1} << kShift,
    kCall       = std::uint64_t{2} << kShift,
    kHereWrap   = std::uint64_t{3} << kShift,
    kHereCall   = std::uint64_t{4} << kShift,
    kWaitStop   = std::uint64_t{5} << kShift,
    kWaitDrop   = std::uint64_t{6} << kShift,
    kWaitNope   = std::uint64_t{7} << kShift,
    kMask       = std::uint64_t{7} << kShift,
  };  // clang-format on

  void Call() noexcept override;
  void Drop() noexcept override;

  virtual void Here(InlineCore& caller, State state) noexcept;
};

}  // namespace yaclib::detail
