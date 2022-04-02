#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/event_deleter.hpp>
#include <yaclib/util/detail/mutex_event.hpp>

#include <cassert>

namespace yaclib {

class WaitGroup {
 public:
  /**
   * Add \ref Future to WaitGroup
   *
   * You can add in the Wait process only from the functors added to this WaitGroup, but we are guaranteed to wait only
   * for those that were added from the functors that were executed before, or added to this WaitGroupx before Wait.
   * You also can't use future before Wait
   * \param f one futures to wait
   */
  template <typename V, typename E>
  void Add(Future<V, E>& f) {
    assert(f.Valid());
    Add(*f.GetCore());
  }

  /**
   * Waiting for the execution of all futures added to WaitGroup
   */
  void Wait();

 private:
  void Add(detail::BaseCore& core);

  detail::AtomicCounter<detail::DefaultEvent, detail::EventDeleter> _event{1};
};

}  // namespace yaclib
