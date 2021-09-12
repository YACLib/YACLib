#pragma once

#include "ref.hpp"

#include <yaclib/container/intrusive_node.hpp>
#include <yaclib/container/intrusive_ptr.hpp>
#include <yaclib/util/counters.hpp>

#include <memory>
#include <utility>

namespace yaclib {

/**
 * \brief Shared callable interface
 */
class IFunc : public IRef {
 public:
  virtual void Call() noexcept = 0;
};

using IFuncPtr = container::intrusive::Ptr<IFunc>;

/**
 * \brief Callable that can be executed in an IExecutor \see IExecutor
 * */
class ITask : public IFunc, public container::intrusive::detail::Node {
 public:
  virtual void Cancel() noexcept = 0;
};

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
  void Cancel() noexcept final {
  }

  void IncRef() noexcept final {
  }

  void DecRef() noexcept final {
    delete this;
  }
};

template <typename Functor>
ITask* MakeUniqueTask(Functor&& functor) {
  return new UniqueTask<std::decay_t<Functor>>{std::forward<Functor>(functor)};
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
