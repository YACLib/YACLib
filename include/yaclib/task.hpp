#pragma once

#include "ref.hpp"

#include <yaclib/container/intrusive_node.hpp>
#include <yaclib/container/intrusive_ptr.hpp>
#include <yaclib/util/counters.hpp>

#include <memory>
#include <utility>

namespace yaclib {

/**
 * \class Shared callable interface
 */
class IFunc : public IRef {
 public:
  virtual void Call() noexcept = 0;
};

using IFuncPtr = container::intrusive::Ptr<IFunc>;

/**
 * \class Callable that can be executed in an IExecutor \see IExecutor
 * */
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
class UniqueTask final : public CallImpl<ITask, Functor> {
 public:
  using CallImpl<ITask, Functor>::CallImpl;

 private:
  void IncRef() noexcept final {
  }

  void DecRef() noexcept final {
    delete this;
  }
};

template <typename Functor>
ITask* MakeUniqueTask(Functor&& functor) {
  using FunctorType = std::remove_reference_t<Functor>;
  return new UniqueTask<FunctorType>{std::forward<Functor>(functor)};
}

}  // namespace detail

/**
 * \brief Create IFunc object from any Callable functor
 * \param functor Callable object
 * */
template <typename Functor>
IFuncPtr MakeFunc(Functor&& functor) {
  using Base = detail::CallImpl<IFunc, std::decay_t<Functor>>;
  return new container::Counter<Base>{std::forward<Functor>(functor)};
}

}  // namespace yaclib
