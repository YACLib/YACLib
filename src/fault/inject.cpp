#include <yaclib/fault/inject.hpp>
#include <yaclib/fault/injector.hpp>

namespace yaclib::detail {

void InjectFault() noexcept {
  GetInjector()->MaybeInject();
}

}  // namespace yaclib::detail
