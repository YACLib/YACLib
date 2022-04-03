#pragma once

#include <yaclib/config.hpp>
#include <yaclib/util/detail/safe_call.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

#include <type_traits>
#include <utility>

namespace yaclib {
namespace detail {

template <typename Functor>
class SharedFunc : public IFunc, public SafeCall<Functor> {
 public:
  using SafeCall<Functor>::SafeCall;

 private:
  void Call() noexcept final {
    SafeCall<Functor>::Call();
  }
};

}  // namespace detail

using IFuncPtr = IntrusivePtr<IFunc>;
/**
 * Create shared \ref IFunc object from any Callable functor
 *
 * \param f Callable object
 */
template <typename Functor>
IFuncPtr MakeFunc(Functor&& f) {
  return MakeIntrusive<detail::SharedFunc<Functor>>(std::forward<Functor>(f));
}

}  // namespace yaclib
