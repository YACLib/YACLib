#include <util/intrusive_list.hpp>
#include <util/mpsc_stack.hpp>

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/atomic.hpp>
#include <yaclib/fault/condition_variable.hpp>
#include <yaclib/fault/mutex.hpp>
#include <yaclib/fault/thread.hpp>

namespace yaclib {
namespace {

thread_local IThreadPool* tlCurrentThreadPool;

class ThreadPool : public IThreadPool {
 public:
  explicit ThreadPool(IThreadFactoryPtr factory, size_t threads)
      : _factory{std::move(factory)}, _threads_count{threads} {
    auto loop = util::MakeFunc([this] {
      tlCurrentThreadPool = this;
      Loop();
      tlCurrentThreadPool = nullptr;
    });
    for (size_t i = 0; i != _threads_count; ++i) {
      _threads.PushBack(_factory->Acquire(loop));
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

  bool Execute(ITask& task) noexcept final {
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
    util::List<ITask> tasks;
    {
      std::lock_guard guard{_m};
      tasks.Append(_tasks);  // TODO(MBkkt) replace Append with Swap
      _stop = true;
    }
    _cv.notify_all();

    while (auto* task = tasks.PopBack()) {
      task->Cancel();
      task->DecRef();
    }
  }

  void Wait() final {
    while (auto* thread = _threads.PopFront()) {
      _factory->Release(IThreadPtr{thread});
    }
  }

  void Loop() noexcept {
    std::unique_lock guard{_m};
    while (true) {
      while (auto* task = _tasks.PopFront()) {
        guard.unlock();
        task->Call();
        task->DecRef();
        if (_refs_flag.fetch_sub(2, std::memory_order_release) == 3) {
          yaclib_std::atomic_thread_fence(std::memory_order_acquire);
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

  alignas(kCacheLineSize) yaclib_std::atomic_size_t _refs_flag{0};
  yaclib_std::mutex _m;
  yaclib_std::condition_variable _cv;
  IThreadFactoryPtr _factory;
  util::List<IThread> _threads;
  util::List<ITask> _tasks;
  const size_t _threads_count;
  bool _stop{false};
};

class SingleThread : public IThreadPool {
 public:
  explicit SingleThread(IThreadFactoryPtr factory) : _factory{std::move(factory)} {
    _thread = _factory->Acquire(util::MakeFunc([this] {
      tlCurrentThreadPool = this;
      Loop();
      tlCurrentThreadPool = nullptr;
    }));
  }

  ~SingleThread() override {
    HardStop();
    Wait();
  }

 private:
  [[nodiscard]] Type Tag() const final {
    return Type::SingleThread;
  }

  bool Execute(ITask& task) noexcept final {
    task.IncRef();
    const auto state = _state.load(std::memory_order_acquire);
    if (state == kStop || state == kHardStop) {
      task.Cancel();
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
      if (_work_counter.load(std::memory_order_acquire) <= 0) {
        _state.store(kStop, std::memory_order_release);
      }
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

  void Wait() final {
    if (_thread) {
      _factory->Release(_thread);
      _thread = nullptr;
    }
  }

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
      head->Cancel();
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
  util::MPSCStack _tasks;
  yaclib_std::atomic<State> _state{kRun};
  yaclib_std::mutex _m;
  yaclib_std::condition_variable _cv;
  alignas(kCacheLineSize) yaclib_std::atomic_int32_t _work_counter{0};
};

}  // namespace

IThreadPool* CurrentThreadPool() noexcept {
  return tlCurrentThreadPool;
}

IThreadPoolPtr MakeThreadPool(size_t threads, IThreadFactoryPtr tf) {
  if (threads == 1) {
    return util::MakeIntrusive<SingleThread, IThreadPool>(std::move(tf));
  }
  return util::MakeIntrusive<ThreadPool, IThreadPool>(std::move(tf), threads);
}

}  // namespace yaclib
