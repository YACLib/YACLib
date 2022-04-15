#include <yaclib/fiber/detail/execution_context.hpp>

extern "C" void yaclib_switch_context(void* from_context, void* to_context);

namespace yaclib {

void ExecutionContext::Setup(StackView stack, Trampoline trampoline, void* arg) {
  if (getcontext(&_context) == -1) {
    abort();
  }
  _context.uc_stack.ss_sp = stack.Begin();
  _context.uc_stack.ss_size = stack.Size();
  makecontext(&_context, (void (*)())trampoline, 1, arg);
}

void ExecutionContext::SwitchTo(ExecutionContext& other) {
  swapcontext(&_context, &other._context);
}

}  // namespace yaclib
