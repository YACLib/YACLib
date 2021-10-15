#pragma once

#include <yaclib/util/func.hpp>
#include <yaclib/util/intrusive_node.hpp>

namespace yaclib {

/**
 * Callable that can be executed in an IExecutor \see IExecutor
 */
class ITask : public util::IFunc, public util::detail::Node {
 public:
  virtual void Cancel() noexcept = 0;
};

namespace detail {

template <typename Functor>
class UniqueTask final : public util::detail::CallImpl<ITask, Functor> {
 public:
  using util::detail::CallImpl<ITask, Functor>::CallImpl;

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
}  // namespace yaclib
