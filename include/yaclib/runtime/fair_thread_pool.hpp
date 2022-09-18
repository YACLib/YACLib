#pragma once

#include <util/intrusive_list.hpp>

#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstddef>
#include <thread>
#include <utility>
#include <vector>
#include <yaclib_std/condition_variable>
#include <yaclib_std/mutex>
#include <yaclib_std/thread>
#include <yaclib_std/thread_local>

namespace yaclib {

class FairThreadPool : public IExecutor {
 public:
  explicit FairThreadPool(std::size_t threads = std::thread::hardware_concurrency()) : _task_count{0} {
    _workers.reserve(threads);
    for (std::size_t i = 0; i != threads; ++i) {
      _workers.emplace_back([&] {
        // SetCurrentThreadPool(*this);
        Loop();
        // SetCurrentThreadPool(MakeInline());
      });
    }
  }

  ~FairThreadPool() noexcept override {
    YACLIB_ERROR(!_workers.empty(), "Explicitly join ThreadPool");
  }

  Type Tag() const noexcept final {
    return Type::FairThreadPool;
  }

  [[nodiscard]] bool WasStop() const noexcept {
    return (_task_count & 1U) != 0U;
  }
  [[nodiscard]] bool WantStop() const noexcept {
    return (_task_count & 2U) != 0U;
  }
  [[nodiscard]] bool NoTasks() const noexcept {
    return (_task_count >> 2U) == 0;
  }
  void Stop(std::unique_lock<yaclib_std::mutex>&& lock) noexcept {
    _task_count |= 1U;
    lock.unlock();
    _idle.notify_all();
  }

  void Submit(Job& task) noexcept final {
    std::unique_lock lock{_m};
    if (WasStop()) {
      lock.unlock();
      task.Drop();
      return;
    }
    _tasks.PushBack(task);
    _task_count += 4;  // Add Task
    lock.unlock();
    _idle.notify_one();
  }

  bool Alive() const noexcept final {
    std::lock_guard lock{_m};
    return !WasStop();
  }

  void SoftStop() {
    std::unique_lock lock{_m};
    if (NoTasks()) {
      Stop(std::move(lock));
    } else {
      _task_count |= 2U;  // Want Stop
    }
  }

  void Stop() {
    Stop(std::unique_lock{_m});
  }

  void HardStop() {
    std::unique_lock lock{_m};
    detail::List tasks{std::move(_tasks)};
    Stop(std::move(lock));
    while (!tasks.Empty()) {
      auto& task = tasks.PopFront();
      static_cast<Job&>(task).Drop();
    }
  }

  // TODO: Rename to join
  void Wait() {
    for (auto& worker : _workers) {
      worker.join();
    }
    _workers.clear();
  }

 private:
  void Loop() noexcept {
    std::unique_lock lock{_m};
    while (true) {
      while (!_tasks.Empty()) {
        auto& task = _tasks.PopFront();
        lock.unlock();
        static_cast<Job&>(task).Call();
        lock.lock();
        _task_count -= 4;  // Pop Task
        if (NoTasks() && WantStop()) {
          return Stop(std::move(lock));
        }
      }
      if (WasStop()) {
        return;
      }
      _idle.wait(lock);
    }
  }

  std::vector<yaclib_std::thread> _workers;
  mutable yaclib_std::mutex _m;
  yaclib_std::condition_variable _idle;
  detail::List _tasks;
  std::size_t _task_count;
};

}  // namespace yaclib
