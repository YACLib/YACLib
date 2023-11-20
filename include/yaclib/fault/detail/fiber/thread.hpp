#pragma once

#include <yaclib/fault/detail/fiber/fiber.hpp>
#include <yaclib/fault/detail/fiber/queue.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>
#include <yaclib/log.hpp>

#include <functional>
#include <thread>

namespace yaclib::detail::fiber {

class Thread {
 public:
  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;

  using id = FiberBase::Id;
  using native_handle_type = void*;

  template <typename... Args>
  explicit Thread(Args&&... args) : _impl{new Fiber<Args...>(std::forward<Args>(args)...)} {
    fault::Scheduler::GetScheduler()->Schedule(_impl);
  }

  Thread() noexcept;
  Thread(Thread&& t) noexcept;
  Thread& operator=(Thread&& t) noexcept;
  ~Thread();

  void swap(Thread& t) noexcept;
  [[nodiscard]] bool joinable() const noexcept;
  void join();
  void detach();

  [[nodiscard]] id get_id() const noexcept;

  native_handle_type native_handle() noexcept;

  static unsigned int hardware_concurrency() noexcept;

  static void SetHardwareConcurrency(unsigned int hardware_concurrency) noexcept;

 private:
  void AfterJoinOrDetach();

  FiberBase* _impl{nullptr};
};

}  // namespace yaclib::detail::fiber
