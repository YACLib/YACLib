#include <util/intrusive_list.hpp>

#include <yaclib/config.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/executor/thread_factory.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/condition_variable.hpp>
#include <yaclib/fault/mutex.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstddef>
#include <utility>

#include <iostream> // debug
#include <yaclib/fault/thread.hpp>


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

  bool Submit(ITask& task) noexcept final {
    std::unique_lock lock{_m};
    if (WasStop()) {
      lock.unlock();
      task.Cancel();
      return false;
    }
    task.IncRef();
    _tasks.PushBack(task);
    _task_count += 4;  // Add Task
    lock.unlock();
    _cv.notify_one();
    return true;
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
    detail::List<ITask> tasks{std::move(_tasks)};
    Stop(std::move(lock));
    while (!tasks.Empty()) {
      auto& task = tasks.PopFront();
      task.Cancel();
      task.DecRef();
    }
  }

  void Wait() final {
    while (!_threads.Empty()) {
      auto* thread = &_threads.PopFront();
      _factory->Release(IThreadPtr{thread});
    }
  }

  void Loop() noexcept {
    std::unique_lock lock{_m};
    while (true) {
      while (!_tasks.Empty()) {
        auto& task = _tasks.PopFront();
        lock.unlock();
        task.Call();
        std::cout << "Call done!(LOOP)" << std::endl;
        using namespace std::chrono_literals; 
        yaclib_std::this_thread::sleep_for(2s);
        task.DecRef();
        std::cout << "DecRef done!(LOOP)" << std::endl; 

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
  detail::List<IThread> _threads;
  detail::List<ITask> _tasks;
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
