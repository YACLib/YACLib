#include <yaclib/fault/inject.hpp>
#include <yaclib/fault/injector.hpp>

namespace yaclib {

detail::Injector* GetInjector() noexcept {
  thread_local static detail::Injector instance;
  return &instance;
}

void InjectFault() noexcept {
  GetInjector()->MaybeInject();
}

}  // namespace yaclib
