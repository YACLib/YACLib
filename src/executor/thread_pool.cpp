#include <util/intrusive_list.hpp>

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/executor/thread_factory.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/condition_variable.hpp>
#include <yaclib/fault/mutex.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstddef>
#include <utility>

namespace yaclib {
namespace {

thread_local IThreadPool* tlCurrentThreadPool;

class ThreadPool : public IThreadPool {
 public:
  explicit ThreadPool(IThreadFactoryPtr factory, std::size_t threads) : _factory{std::move(factory)}, _task_count{0} {
    auto loop = MakeFunc([this] {
      tlCurrentThreadPool = this;
      Loop();
      tlCurrentThreadPool = nullptr;
    });
    for (std::size_t i = 0; i != threads; ++i) {
      auto* thread = _factory->Acquire(loop);
      YACLIB_ERROR(thread == nullptr, "Acquired from thread factory thread is null");
      _threads.PushBack(*thread);
    }
  }

  ~ThreadPool() override {
    HardStop();
    Wait();
  }

 private:
  Type Tag() const final {
    return Type::ThreadPool;
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
    _cv.notify_all();
  }

  void Submit(ITask& task) noexcept final {
    std::unique_lock lock{_m};
    if (WasStop()) {
      lock.unlock();
      task.Cancel();
      return;
    }
    _tasks.PushBack(task);
    _task_count += 4;  // Add Task
    lock.unlock();
    _cv.notify_one();
  }

  void SoftStop() final {
    std::unique_lock lock{_m};
    if (NoTasks()) {
      Stop(std::move(lock));
    } else {
      _task_count |= 2U;  // Want Stop
    }
  }

  void Stop() final {
    Stop(std::unique_lock{_m});
  }

  void HardStop() final {
    std::unique_lock lock{_m};
    detail::List tasks{std::move(_tasks)};
    Stop(std::move(lock));
    while (!tasks.Empty()) {
      auto& task = tasks.PopFront();
      static_cast<ITask&>(task).Cancel();
    }
  }

  void Wait() final {
    while (!_threads.Empty()) {
      auto& thread = _threads.PopFront();
      _factory->Release(&static_cast<IThread&>(thread));
    }
  }

  void Loop() noexcept {
    std::unique_lock lock{_m};
    while (true) {
      while (!_tasks.Empty()) {
        auto& task = _tasks.PopFront();
        lock.unlock();
        static_cast<ITask&>(task).Call();
        lock.lock();
        _task_count -= 4;  // Pop Task
        if (NoTasks() && WantStop()) {
          return Stop(std::move(lock));
        }
      }
      if (WasStop()) {
        return;
      }
      _cv.wait(lock);
    }
  }

  yaclib_std::mutex _m;
  yaclib_std::condition_variable _cv;
  IThreadFactoryPtr _factory;
  detail::List _threads;
  detail::List _tasks;
  std::size_t _task_count;
};

}  // namespace

IThreadPool* CurrentThreadPool() noexcept {
  return tlCurrentThreadPool;
}

IThreadPoolPtr MakeThreadPool(std::size_t threads, IThreadFactoryPtr tf) {
  // TODO(MBkkt) Optimize this case
  // if (threads == 1) {
  //   return MakeIntrusive<SingleThread, IThreadPool>(std::move(tf));
  // }
  return MakeIntrusive<ThreadPool, IThreadPool>(std::move(tf), threads);
}

}  // namespace yaclib
