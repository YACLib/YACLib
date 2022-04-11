#pragma once

#include <yaclib/executor/task.hpp>
#include <yaclib/util/detail/safe_call.hpp>

#include <utility>

namespace yaclib::detail {

template <typename Functor>
class UniqueTask final : public ITask, public SafeCall<Functor> {
 public:
  using SafeCall<Functor>::SafeCall;

 private:
  void Call() noexcept final {
    SafeCall<Functor>::Call();
    DecRef();
  }
  void Cancel() noexcept final {
    DecRef();
  }

  void IncRef() noexcept final {
  }

  void DecRef() noexcept final {
    delete this;
  }
};

template <typename Functor>
ITask* MakeUniqueTask(Functor&& functor) {
  return new UniqueTask<Functor>{std::forward<Functor>(functor)};
}

}  // namespace yaclib::detail
