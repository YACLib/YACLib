#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/detail/wait_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/util/counters.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

class WaitGroup {
 public:
  WaitGroup();

  /**
   * Add \ref Future to WaitGroup
   * You can add in the Wait process only from the functors added to this WaitGroup, but we are guaranteed to wait only
   * for those that were added from the functors that were executed before, or added to this WaitGroupx before Wait.
   * You also can't use future before Wait
   *
   * \param f one futures to wait
   */
  template <typename Future>
  void Add(Future& f) {
    f._core->SetWaitCallback(_callback);
  }

  /**
   * Waiting for the execution of all futures added to WaitGroup
   */
  void Wait();

 private:
  util::Counter<detail::WaitCore, detail::WaitCoreDeleter> _callback{};
};

}  // namespace yaclib
