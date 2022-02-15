#include <yaclib/fault/detail/inject_fault.hpp>

namespace yaclib::detail {

Yielder* GetYielder() {
  static Yielder instance;
  return &instance;
}

void InjectFault() {
  GetYielder()->MaybeYield();
}

}  // namespace yaclib::detail
