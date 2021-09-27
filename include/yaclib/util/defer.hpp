#pragma once

#include <type_traits>
#include <utility>

namespace yaclib::util {
namespace detail {

/**
 * DeferAction allows you to ensure something gets run at the end of a scope
 */
template <typename F>
class DeferAction {
 public:
  static_assert(!std::is_reference<F>::value && !std::is_const<F>::value && !std::is_volatile<F>::value,
                "Final_action should store its callable by value");

  explicit DeferAction(F f) noexcept : _f(std::move(f)) {
  }

  DeferAction(DeferAction&& other) noexcept : _f(std::move(other._f)), _invoke(std::exchange(other._invoke, false)) {
  }

  DeferAction(const DeferAction&) = delete;
  DeferAction& operator=(const DeferAction&) = delete;
  DeferAction& operator=(DeferAction&&) = delete;

  ~DeferAction() noexcept {
    if (_invoke) {
      _f();
    }
  }

 private:
  F _f;
  bool _invoke{true};
};

}  // namespace detail

/**
 * defer() - convenience function to generate a DeferAction
 */
template <typename F>
[[nodiscard]] detail::DeferAction<typename std::remove_cv<typename std::remove_reference<F>::type>::type> defer(
    F&& f) noexcept {
  return detail::DeferAction<typename std::remove_cv<typename std::remove_reference<F>::type>::type>(
      std::forward<F>(f));
}

}  // namespace yaclib::util
