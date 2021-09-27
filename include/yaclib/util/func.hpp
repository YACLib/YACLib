#pragma once

#include <yaclib/util/counters.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

#include <memory>
#include <utility>

namespace yaclib::util {

/**
 * Shared callable interface
 */
class IFunc : public IRef {
 public:
  virtual void Call() noexcept = 0;
};

using IFuncPtr = Ptr<IFunc>;

namespace detail {

template <typename Interface, typename Functor>
class CallImpl : public Interface {
 public:
  explicit CallImpl(Functor&& functor) : _functor{std::move(functor)} {
  }

  explicit CallImpl(const Functor& functor) : _functor{functor} {
  }

 private:
  void Call() noexcept final {
    try {
      _functor();
    } catch (...) {
      // TODO(MBkkt): create issue
    }
  }

  Functor _functor;
};

}  // namespace detail

/**
 * Create \ref IFunc object from any Callable functor
 *
 * \param f Callable object
 */
template <typename Functor>
IFuncPtr MakeFunc(Functor&& f) {
  return new util::Counter<detail::CallImpl<IFunc, std::decay_t<Functor>>>{std::forward<Functor>(f)};
}

}  // namespace yaclib::util
