#pragma once

#include <yaclib/container/intrusive_node.hpp>

#include <functional>
#include <memory>

namespace yaclib {

class ITask : public container::intrusive::detail::Node<ITask> {
 public:
  virtual void Call() = 0;

  virtual ~ITask() = default;
};

using ITaskPtr = std::unique_ptr<ITask>;

using Functor = std::shared_ptr<ITask>;

namespace detail {

template <typename Functor>
class Task final : public ITask {
 public:
  Task(Functor&& f) : _f{std::move(f)} {
  }

  Task(const Functor& f) : _f{f} {
  }

 private:
  void Call() noexcept final {
    try {
      _f();
    } catch (...) {
      // TODO(MBkkt): create issue
    }
  }

  Functor _f;
};

}  // namespace detail

template <typename Functor>
ITaskPtr MakeTask(Functor&& f) {
  using FunctorT = std::remove_reference_t<Functor>;
  return std::make_unique<detail::Task<FunctorT>>(std::forward<Functor>(f));
}

template <typename Functor>
yaclib::Functor MakeFunctor(Functor&& f) {
  using FunctorT = std::remove_reference_t<Functor>;
  return std::make_shared<detail::Task<FunctorT>>(std::forward<Functor>(f));
}

}  // namespace yaclib
