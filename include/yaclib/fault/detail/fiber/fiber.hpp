#pragma once

#include <yaclib/fault/detail/fiber/fiber_base.hpp>

#include <unordered_map>

namespace yaclib::detail::fiber {

template <typename... Args>
using FuncState = std::tuple<typename std::decay_t<Args>...>;

template <typename... Args>
class Fiber final : public FiberBase {
 public:
  // TODO(myannyax): add tests
  Fiber(Args&&... args) : _func_state(std::forward<Args>(args)...) {
    _context.Setup(_stack.GetAllocation(), Trampoline, this);
  }

  [[noreturn]] static void Trampoline(void* arg) noexcept {
    auto* coroutine = reinterpret_cast<Fiber*>(arg);
    try {
      Helper(coroutine->_func_state, std::index_sequence_for<FuncState<Args...>>{});
    } catch (...) {
      coroutine->_exception = std::current_exception();
    }

    coroutine->Complete();
  }

  ~Fiber() final = default;

 private:
  template <typename Tuple, std::size_t... I>
  static auto Helper(Tuple& a, std::index_sequence<I...>) {
    std::__invoke(std::move(std::get<I>(a))...);
  }

  FuncState<Args...> _func_state;
};

}  // namespace yaclib::detail::fiber
