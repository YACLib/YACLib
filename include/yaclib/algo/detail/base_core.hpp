#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

#include <yaclib_std/atomic>

namespace yaclib::detail {

class BaseCore : public InlineCore {
  static constexpr std::uint64_t kShift = 61;

 public:
  enum State : std::uint64_t {  // clang-format off
    kEmpty    = std::uint64_t{0} << kShift,
    kResult   = std::uint64_t{1} << kShift,
    kWaitDrop = std::uint64_t{2} << kShift,
    kWaitNope = std::uint64_t{3} << kShift,
    kInline   = std::uint64_t{4} << kShift,
    kCall     = std::uint64_t{5} << kShift,
    kMask     = std::uint64_t{7} << kShift,
  };  // clang-format on

  [[nodiscard]] bool Empty() const noexcept;

  void SetInline(InlineCore& callback) noexcept;

  void SetCall(BaseCore& callback) noexcept;

  [[nodiscard]] bool SetCallback(InlineCore& callback, State state) noexcept;

  void StoreCallback(InlineCore& callback, State state) noexcept;

  [[nodiscard]] bool Reset() noexcept;

  template <bool SymmetricTransfer>
#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
  using ReturnT = std::conditional_t<SymmetricTransfer, yaclib_std::coroutine_handle<>, void>;
#else
  using ReturnT = void;
#endif

  template <bool SymmetricTransfer>
  ReturnT<SymmetricTransfer> SetResult() noexcept;

 protected:
  explicit BaseCore(State callback) noexcept;

  yaclib_std::atomic_uint64_t _callback;

 public:
  IExecutorPtr _executor{NoRefTag{}, &MakeInline()};

 private:
  void Submit(BaseCore& callback) noexcept;
};

}  // namespace yaclib::detail
