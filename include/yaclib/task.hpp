#pragma once

#include <yaclib/container/intrusive_node.hpp>

#include <functional>
#include <memory>

namespace yaclib {

class IFunc {
 public:
  virtual void Call() = 0;

  virtual ~IFunc() = default;
};

using IFuncPtr = std::shared_ptr<IFunc>;

class ITask : public IFunc, public container::intrusive::detail::Node {};

using ITaskPtr = std::unique_ptr<ITask>;

namespace detail {

template <typename Interface, typename Functor>
class Impl final : public Interface {
 public:
  Impl(Functor&& functor) : _functor{std::move(functor)} {
  }

  Impl(const Functor& functor) : _functor{functor} {
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

template <typename Functor>
ITaskPtr MakeTask(Functor&& functor) {
  return std::make_unique<
      detail::Impl<ITask, std::remove_reference_t<Functor>>>(
      std::forward<Functor>(functor));
}

template <typename Functor>
IFuncPtr MakeFunc(Functor&& functor) {
  return std::make_shared<
      detail::Impl<IFunc, std::remove_reference_t<Functor>>>(
      std::forward<Functor>(functor));
}

}  // namespace yaclib
