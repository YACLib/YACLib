#include <yaclib/fault/detail/antagonist/inject_fault.hpp>

namespace yaclib::detail {

constexpr int kFreq = 16;

Yielder* GetYielder() {
  static Yielder instance(kFreq);
  return &instance;
}

void InjectFault() {
  GetYielder()->MaybeYield();
}

}  // namespace yaclib::detail
