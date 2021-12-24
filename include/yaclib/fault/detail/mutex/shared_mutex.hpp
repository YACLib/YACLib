#pragma once

#include <yaclib/fault/detail/antagonist/inject_fault.hpp>

#include <cassert>
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
  std::shared_mutex _m;
  std::shared_mutex _helper_m;  // for _shared_owners
  // TODO(myannyax) yaclib wrapper
  std::atomic<yaclib_std::thread::id> _exclusive_owner{yaclib::detail::kInvalidThreadId};
  // TODO(myannyax) remove / change?
  std::unordered_set<yaclib_std::thread::id> _shared_owners;
};

}  // namespace yaclib::detail
