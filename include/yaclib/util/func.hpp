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

template <typename Interface, typename Func>
class CallImpl : public Interface {
 public:
  explicit CallImpl(Func&& f) : _func{std::move(f)} {
  }

  explicit CallImpl(const Func& f) : _func{f} {
  }

 private:
  void Call() noexcept final {
    try {
      _func();
    } catch (...) {
      // TODO(MBkkt): create issue
    }
  }

  Func _func;
};

}  // namespace detail

/**
 * Create \ref IFunc object from any Callable functor
 *
 * \param f Callable object
 */
template <typename Func>
IFuncPtr MakeFunc(Func&& f) {
  return new util::Counter<detail::CallImpl<IFunc, std::decay_t<Func>>>{std::forward<Func>(f)};
}

}  // namespace yaclib::util
