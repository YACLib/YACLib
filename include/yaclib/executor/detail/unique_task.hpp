#pragma once

#include <yaclib/config.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/util/detail/safe_call.hpp>

#include <utility>

namespace yaclib::detail {

template <typename Functor>
class UniqueTask final : public SafeCall<ITask, Functor> {
 public:
  using SafeCall<ITask, Functor>::SafeCall;

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
  return new UniqueTask<Functor>{std::forward<Functor>(functor)};
}

}  // namespace yaclib::detail
