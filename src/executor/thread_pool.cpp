#include <container/intrusive_list.hpp>

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace yaclib::executor {
namespace {

thread_local IThreadPool* tlCurrentThreadPool;

class ThreadPool final : public IThreadPool {
 public:
  ThreadPool(IThreadFactoryPtr factory, size_t threads)
      : _factory{std::move(factory)}, _threads_count{threads} {
    auto loop = MakeFunc([this] {
      tlCurrentThreadPool = this;
      Loop();
      tlCurrentThreadPool = nullptr;
    });
    for (size_t i = 0; i != _threads_count; ++i) {
      _threads.PushBack(_factory->Acquire(loop).release());
    }
  }

  ~ThreadPool() final {
    HardStop();
    Wait();
  }

  void Execute(ITask& task) final {
    _refs_flag.fetch_add(2, std::memory_order_relaxed);
    {
      std::lock_guard guard{_m};
      if (_stop) {
        return;
      }
      task.Acquire();
      _tasks.PushBack(&task);
    }
    _cv.notify_one();
  }

  void SoftStop() final {
    if (_refs_flag.fetch_add(1, std::memory_order_acq_rel) == 0) {
      Stop();
    }
  }

  void Stop() final {
    {
      std::lock_guard guard{_m};
      _stop = true;
    }
    _cv.notify_all();
  }

  void HardStop() final {
    container::intrusive::List<ITask> tasks;
    {
      std::lock_guard guard{_m};
      tasks.Append(_tasks);
      _stop = true;
    }
    _cv.notify_all();

    while (auto task = tasks.PopBack()) {
      task->Release();
    }
  }

  void Wait() final {
    while (auto thread = _threads.PopFront()) {
      _factory->Release(executor::IThreadPtr{thread});
    }
  }

 private:
  void Loop() noexcept {
    std::unique_lock guard{_m};
    while (true) {
      while (auto task = _tasks.PopFront()) {
        guard.unlock();
        task->Call();
        task->Release();
        if (_refs_flag.fetch_sub(2, std::memory_order_release) == 3) {
          std::atomic_thread_fence(std::memory_order_acquire);
          Stop();
          return;
        }
        guard.lock();
      }
      if (_stop) {
        return;
      }
      _cv.wait(guard);
    }
  }

  alignas(64) std::atomic_size_t _refs_flag{0};
  std::mutex _m;
  std::condition_variable _cv;
  IThreadFactoryPtr _factory;
  container::intrusive::List<IThread> _threads;
  container::intrusive::List<ITask> _tasks;
  const size_t _threads_count;
  bool _stop{false};
};

}  // namespace

IThreadPool* CurrentThreadPool() noexcept {
  return tlCurrentThreadPool;
}

IThreadPoolPtr MakeThreadPool(size_t threads, IThreadFactoryPtr factory) {
  return std::make_shared<ThreadPool>(std::move(factory), threads);
}

}  // namespace yaclib::executor
