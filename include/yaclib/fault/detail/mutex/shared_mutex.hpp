#pragma once

#include <yaclib/fault/detail/antagonist/inject_fault.hpp>

#include <shared_mutex>
#include <unordered_set>

namespace yaclib::detail {

class SharedMutex {
 public:
  SharedMutex() = default;
  ~SharedMutex() noexcept = default;

  SharedMutex(const SharedMutex&) = delete;
  SharedMutex& operator=(const SharedMutex&) = delete;

  void lock();
  bool try_lock() noexcept;
  void unlock() noexcept;

  void lock_shared();
  bool try_lock_shared() noexcept;
  void unlock_shared() noexcept;

  // TODO(myannyax) no handle (my local header has them commented)?

 private:
#ifdef YACLIB_FIBER
  kek _m;
#else
  ::std::shared_mutex _m;
  ::std::shared_mutex _helper_m;  // for _shared_owners
#endif
  // TODO(myannyax) yaclib wrapper
  ::std::atomic<yaclib::std::thread::id> _exclusive_owner{yaclib::detail::kInvalidThreadId};
  // TODO(myannyax) remove / change?
  ::std::unordered_set<yaclib::std::thread::id> _shared_owners;
};

}  // namespace yaclib::detail
