#include <container/intrusive_list.hpp>
#include <container/mpsc_stack.hpp>

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace yaclib::executor {
namespace {

thread_local IThreadPool* tlCurrentThreadPool;

class ThreadPool : public IThreadPool {
 public:
  explicit ThreadPool(IThreadFactoryPtr factory, size_t threads)
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

  ~ThreadPool() override {
    HardStop();
    Wait();
  }

 private:
  Type Tag() const final {
    return Type::ThreadPool;
  }

  bool Execute(ITask& task) final {
    _refs_flag.fetch_add(2, std::memory_order_relaxed);
    task.IncRef();
    std::unique_lock guard{_m};
    if (_stop) {
      guard.unlock();
      task.Cancel();
      task.DecRef();
      return false;
    }
    _tasks.PushBack(&task);
    guard.unlock();
    _cv.notify_one();
    return true;
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
      tasks.Append(_tasks);  // TODO(MBkkt) replace Append with Swap
      _stop = true;
    }
    _cv.notify_all();

    while (auto task = tasks.PopBack()) {
      task->Cancel();
      task->DecRef();
    }
  }

  void Wait() final {
    while (auto thread = _threads.PopFront()) {
      _factory->Release(executor::IThreadPtr{thread});
    }
  }

  void Loop() noexcept {
    std::unique_lock guard{_m};
    while (true) {
      while (auto task = _tasks.PopFront()) {
        guard.unlock();
        task->Call();
        task->DecRef();
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

  alignas(kCacheLineSize) std::atomic_size_t _refs_flag{0};
  std::mutex _m;
  std::condition_variable _cv;
  IThreadFactoryPtr _factory;
  container::intrusive::List<IThread> _threads;
  container::intrusive::List<ITask> _tasks;
  const size_t _threads_count;
  bool _stop{false};
};

class SingleThread : public IThreadPool {
 public:
  explicit SingleThread(IThreadFactoryPtr factory) : _factory{std::move(factory)} {
    auto loop = MakeFunc([this] {
      tlCurrentThreadPool = this;
      Loop();
      tlCurrentThreadPool = nullptr;
    });
    _thread = _factory->Acquire(loop);
  }

  ~SingleThread() override {
    HardStop();
    Wait();
  }

  Type Tag() const final {
    return Type::SingleThread;
  }

  bool Execute(ITask& task) final {
    task.IncRef();
    const auto state = _state.load(std::memory_order_acquire);
    if (state == kStop || state == kHardStop) {
      task.DecRef();
      return false;
    }
    _tasks.Put(&task);

    if (_work_counter.fetch_add(1, std::memory_order_acq_rel) == 0) {
      _m.lock();
      _m.unlock();
      _cv.notify_one();
    }
    return true;
  }

  void SoftStop() final {
    {
      std::lock_guard guard{_m};
      _state.store(kSoftStop, std::memory_order_release);
    }
    _cv.notify_one();
  }

  void Stop() final {
    {
      std::lock_guard guard{_m};
      _state.store(kStop, std::memory_order_release);
    }
    _cv.notify_one();
  }

  void HardStop() final {
    {
      std::lock_guard guard{_m};
      _state.store(kHardStop, std::memory_order_release);
    }
    _cv.notify_one();
  }

  void Wait() {
    _factory->Release(std::move(_thread));
  }

 private:
  void Loop() noexcept {
    while (true) {
      int32_t size = 0;
      do {
        size = 0;

        auto state = _state.load(std::memory_order_acquire);
        auto nodes{_tasks.TakeAllFIFO()};
        if ((state == kHardStop || state == kStop) && nodes == nullptr) {
          return;
        }
        state = _state.load(std::memory_order_acquire);
        if (state == kSoftStop && nodes == nullptr) {
          Stop();
        }

        auto task = static_cast<ITask*>(nodes);
        while (task != nullptr) {
          state = _state.load(std::memory_order_acquire);
          if (state == kHardStop) {
            KillTasks(task);
            KillTasks(static_cast<ITask*>(_tasks.TakeAllLIFO()));
            return;
          }
          auto next = static_cast<ITask*>(task->_next);
          task->Call();
          task->DecRef();
          task = next;
          ++size;
        }

      } while (_work_counter.fetch_sub(size, std::memory_order_acq_rel) > size);
      std::unique_lock guard{_m};
      if (_work_counter.load(std::memory_order_acquire) <= 0 && _state.load(std::memory_order_acquire) == kRun) {
        _cv.wait(guard);
      }
    }
  }

  static void KillTasks(ITask* head) {
    while (head != nullptr) {
      auto next = static_cast<ITask*>(head->_next);
      head->DecRef();
      head = next;
    }
  }

  enum State {
    kRun = 0,
    kStop,
    kSoftStop,
    kHardStop,
  };

  IThreadFactoryPtr _factory;
  IThreadPtr _thread;
  container::intrusive::MPSCStack _tasks;
  std::atomic<State> _state{kRun};
  std::mutex _m;
  std::condition_variable _cv;
  alignas(kCacheLineSize) std::atomic_int32_t _work_counter{0};
};

}  // namespace

IThreadPool* CurrentThreadPool() noexcept {
  return tlCurrentThreadPool;
}

IThreadPoolPtr MakeThreadPool(size_t threads, IThreadFactoryPtr factory) {
  if (threads == 1) {
    return new container::Counter<SingleThread>{std::move(factory)};
  }
  return new container::Counter<ThreadPool>{std::move(factory), threads};
}

}  // namespace yaclib::executor
