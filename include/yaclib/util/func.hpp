#pragma once

#include <yaclib/util/detail/safe_call.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

#include <type_traits>
#include <utility>

namespace yaclib {

/**
 * Shared callable interface
 */
class IFunc : public IRef {
 public:
  virtual void Call() noexcept = 0;
};

using IFuncPtr = IntrusivePtr<IFunc>;

/**
 * Create \ref IFunc object from any Callable functor
 *
 * \param f Callable object
 */
template <typename Functor>
IFuncPtr MakeFunc(Functor&& f) {
  return MakeIntrusive<detail::SafeCall<IFunc, Functor>, IFunc>(std::forward<Functor>(f));
}

}  // namespace yaclib
