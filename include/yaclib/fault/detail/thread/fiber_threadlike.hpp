#pragma once

#include <yaclib/fault/detail/fiber/queue.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/func.hpp>

#include <functional>
#include <thread>

namespace yaclib::detail {

class FiberThreadlike {
 public:
  FiberThreadlike(const FiberThreadlike&) = delete;
  FiberThreadlike& operator=(const FiberThreadlike&) = delete;

  using id = Fiber::Id;
  using native_handle_type = std::thread::native_handle_type;

  template <class Fp, class... Args>
  inline explicit FiberThreadlike(Fp&& f, Args&&... args) : _join_queue(new FiberQueue()) {
    yaclib::IFuncPtr func = yaclib::MakeFunc([&, f = std::forward<Fp>(f)]() mutable {
      f(std::forward(args)...);
    });
    _impl = MakeIntrusive<Fiber>(func);
    _impl->SetCompleteCallback(yaclib::MakeFunc([queue = _join_queue]() mutable {
      queue->NotifyAll();
    }));
    GetScheduler()->Run(_impl);
  }

  FiberThreadlike() noexcept;
  FiberThreadlike(FiberThreadlike&& t) noexcept = default;
  FiberThreadlike& operator=(FiberThreadlike&& t) noexcept = default;

  void swap(FiberThreadlike& t) noexcept;
  [[nodiscard]] bool joinable() const noexcept;
  void join();
  void detach();

  [[nodiscard]] id get_id() const noexcept;

  native_handle_type native_handle() noexcept;

  // TODO(myannyax) don't use auto?
  static unsigned int hardware_concurrency() noexcept;

 private:
  IntrusivePtr<Fiber> _impl;
  FiberQueue* _join_queue;
};

extern const Fiber::Id kInvalidThreadId;

}  // namespace yaclib::detail
