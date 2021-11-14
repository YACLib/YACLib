#pragma once

#include <yaclib/async/detail/wait_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/util/counters.hpp>

namespace yaclib {

class WaitGroup {
 public:
  WaitGroup();

  /**
   * Add \ref Future to WaitGroup
   *
   * You can add in the Wait process only from the functors added to this WaitGroup, but we are guaranteed to wait only
   * for those that were added from the functors that were executed before, or added to this WaitGroupx before Wait.
   * You also can't use future before Wait
   * \param f one futures to wait
   */
  template <typename T>
  void Add(Future<T>& f) {
    f.GetCore()->SetWait(_callback);
  }
  void Add(Future<void>&& f);

  /**
   * Waiting for the execution of all futures added to WaitGroup
   */
  void Wait();

 private:
  util::Counter<detail::WaitCore, detail::WaitCoreDeleter> _callback{};
};

extern template void WaitGroup::Add<void>(Future<void>& f);

}  // namespace yaclib
