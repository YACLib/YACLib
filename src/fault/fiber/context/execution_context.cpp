#include <yaclib/fault/detail/fiber/execution_context.hpp>

#include <cstdlib>

namespace yaclib::detail::fiber {

void ExecutionContext::Setup(Allocation stack, Trampoline trampoline, void* arg) {
  if (getcontext(&_context) == -1) {
    abort();
  }
  _stack = stack;
  _context.uc_stack.ss_sp = stack.start;
  _context.uc_stack.ss_size = stack.size;
  makecontext(&_context, reinterpret_cast<void (*)()>(trampoline), 1, arg);
}

void ExecutionContext::SwitchTo(ExecutionContext& other) {
#ifdef YACLIB_ASAN
  void* fake_stack{nullptr};
  const void* bottom_old{nullptr};
  std::size_t size_old{0};
  __sanitizer_start_switch_fiber(&fake_stack, other._stack.start, other._stack.size);
#endif
  swapcontext(&_context, &other._context);
#ifdef YACLIB_ASAN
  __sanitizer_finish_switch_fiber(fake_stack, &bottom_old, &size_old);
#endif
}

void ExecutionContext::Exit(ExecutionContext& other) {
#ifdef YACLIB_ASAN
  __sanitizer_start_switch_fiber(nullptr, other._stack.start, other._stack.size);
#endif
  swapcontext(&_context, &other._context);
}

}  // namespace yaclib::detail::fiber
