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

void ExecutionContext::Start() {
#ifdef YACLIB_ASAN
  __sanitizer_finish_switch_fiber(nullptr, &_old_stack_bottom, &_old_stack_size);
#endif
}

void ExecutionContext::SwitchTo(ExecutionContext& other) {
#ifdef YACLIB_ASAN
  void* fake_stack{nullptr};
  __sanitizer_start_switch_fiber(&fake_stack, other._context.uc_stack.ss_sp, other._context.uc_stack.ss_size);
#endif
  swapcontext(&_context, &other._context);
#ifdef YACLIB_ASAN
  __sanitizer_finish_switch_fiber(fake_stack, nullptr, nullptr);
#endif
}

void ExecutionContext::Exit(ExecutionContext& other) {
  __sanitizer_start_switch_fiber(nullptr, _old_stack_bottom, _old_stack_size);
  setcontext(&other._context);
}

}  // namespace yaclib::detail::fiber
