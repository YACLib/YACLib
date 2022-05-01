#include <yaclib/fault/detail/fiber/execution_context.hpp>

namespace yaclib::detail {

void ExecutionContext::Setup(Allocation stack, Trampoline trampoline, void* arg) {
  if (getcontext(&_context) == -1) {
    abort();
  }
  _context.uc_stack.ss_sp = stack.start;
  _context.uc_stack.ss_size = stack.size;
  makecontext(&_context, reinterpret_cast<void (*)()>(trampoline), 1, arg);
}

void ExecutionContext::SwitchTo(ExecutionContext& other) {
  swapcontext(&_context, &other._context);
}

}  // namespace yaclib::detail
