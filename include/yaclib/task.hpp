#pragma once

#include "ref.hpp"

#include <yaclib/container/intrusive_node.hpp>

#include <memory>
#include <utility>

namespace yaclib {

class IFunc : public IRef {
 public:
  virtual void Call() = 0;
};

using IFuncPtr = std::shared_ptr<IFunc>;

class ITask : public IFunc, public container::intrusive::detail::Node {};

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

template <typename Functor>
class FunctorFunc final : public CallImpl<IFunc, Functor> {
 public:
  using CallImpl<IFunc, Functor>::CallImpl;

 private:
  void Acquire() noexcept final {
  }

  void Release() noexcept final {
  }
};

template <typename Functor>
class FunctorTask final : public CallImpl<ITask, Functor> {
 public:
  using CallImpl<ITask, Functor>::CallImpl;

 private:
  void Acquire() noexcept final {
  }

  void Release() noexcept final {
    delete this;
  }
};

template <typename Functor>
ITask* MakeFunctorTask(Functor&& functor) {
  using FunctorType = std::remove_reference_t<Functor>;
  return new FunctorTask<FunctorType>{std::forward<Functor>(functor)};
}

}  // namespace detail

template <typename Functor>
IFuncPtr MakeFunc(Functor&& functor) {
  using FunctorType = std::remove_reference_t<Functor>;
  return std::make_shared<detail::FunctorFunc<FunctorType>>(
      std::forward<Functor>(functor));
}

}  // namespace yaclib
