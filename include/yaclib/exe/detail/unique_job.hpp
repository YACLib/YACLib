#pragma once

#include <yaclib/exe/job.hpp>
#include <yaclib/util/detail/safe_call.hpp>

#include <utility>

namespace yaclib::detail {

template <typename Func>
class UniqueJob final : public Job, public SafeCall<Func> {
 public:
  using SafeCall<Func>::SafeCall;

 private:
  void Call() noexcept final {
    SafeCall<Func>::Call();
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

template <typename Func>
Job* MakeUniqueJob(Func&& f) {
  return new UniqueJob<Func>{std::forward<Func>(f)};
}

}  // namespace yaclib::detail
