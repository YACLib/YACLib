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
  void Call() noexcept final;
  void Drop() noexcept final;
};

template <typename Func>
void UniqueJob<Func>::Call() noexcept {
  SafeCall<Func>::Call();
  Drop();
}

template <typename Func>
void UniqueJob<Func>::Drop() noexcept {
  delete this;
}

template <typename Func>
Job* MakeUniqueJob(Func&& f) {
  return new UniqueJob<decltype(std::forward<Func>(f))>{std::forward<Func>(f)};
}

}  // namespace yaclib::detail
