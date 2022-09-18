#pragma once

#include <yaclib/fault/detail/fiber/fiber_base.hpp>

#include <functional>

namespace yaclib::detail::fiber {

template <typename... Args>
using FuncState = std::tuple<typename std::decay_t<Args>...>;

template <typename... Args>
class Fiber final : public FiberBase {
 public:
  // TODO(myannyax) add tests
  Fiber(Args&&... args) : _func(std::forward<Args>(args)...) {
    _context.Setup(_stack.GetAllocation(), Trampoline, this);
  }

  static void Trampoline(void* arg) noexcept {
    auto& fiber = *reinterpret_cast<Fiber*>(arg);
    fiber.Start();
    try {
      Helper(fiber._func, std::index_sequence_for<FuncState<Args...>>{});
    } catch (...) {
      fiber._exception = std::current_exception();
    }
    fiber.Exit();
  }

  ~Fiber() final = default;

 private:
  template <typename Tuple, std::size_t... I>
  static auto Helper(Tuple& a, std::index_sequence<I...>) {
    std::invoke(std::move(std::get<I>(a))...);
  }

  FuncState<Args...> _func;
};

}  // namespace yaclib::detail::fiber
