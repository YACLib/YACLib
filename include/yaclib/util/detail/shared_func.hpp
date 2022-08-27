#pragma once

#include <yaclib/util/detail/safe_call.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

#include <type_traits>
#include <utility>

namespace yaclib {
namespace detail {

template <typename Func>
class SharedFunc : public IFunc, public SafeCall<Func> {
 public:
  using SafeCall<Func>::SafeCall;

 private:
  void Call() noexcept final {
    SafeCall<Func>::Call();
  }
};

}  // namespace detail

using IFuncPtr = IntrusivePtr<IFunc>;

/**
 * Create shared \ref IFunc object from any Callable func
 *
 * \param f Callable object
 */
template <typename Func>
IFuncPtr MakeFunc(Func&& f) {
  return MakeShared<detail::SharedFunc<Func>>(1, std::forward<Func>(f));
}

}  // namespace yaclib
