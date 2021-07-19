#include <container/intrusive_list.hpp>

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <condition_variable>
#include <mutex>
#include <thread>

namespace yaclib::executor {

namespace {

thread_local IThreadPool* tlCurrentThreadPool;

class ThreadPool final : public IThreadPool {
 public:
  explicit ThreadPool(size_t threads)
      : ThreadPool{MakeThreadFactory(), threads, threads} {
  }

  ThreadPool(IThreadFactoryPtr factory, size_t cache_threads,
             size_t max_threads)
      : _factory(std::move(factory)),
        _cache_threads{cache_threads},
        _max_threads{max_threads} {
    auto loop_functor = MakeFunc([this] {
      tlCurrentThreadPool = this;
      Loop();
      tlCurrentThreadPool = nullptr;
    });
    for (size_t i = 0; i != _cache_threads; ++i) {
      _threads.PushBack(_factory->Acquire(loop_functor).release());
    }
  }

  ~ThreadPool() final {
    Cancel();
    Wait();
  }

  void Execute(ITaskPtr task) final {
    {
      std::lock_guard guard{_m};
      if (_stopped) {
        return;
      }
      _tasks.PushBack(task.release());
    }
    _cv.notify_one();
  }

  void Stop() final {
    // TODO(kononovk): #2 After call: сalling Execute() for all,
    //  expect this ThreadPool tasks, does nothing
  }

  void Close() final {
    {
      std::lock_guard guard{_m};
      _stopped = true;
    }
    _cv.notify_all();
  }

  void Cancel() final {
    container::intrusive::List<ITask> tasks;
    {
      std::lock_guard guard{_m};
      tasks.Append(_tasks);
      _stopped = true;
    }

    while (!tasks.IsEmpty()) {
      ITaskPtr{tasks.PopBack()};  // for delete
    }

    _cv.notify_all();
  }

  void Wait() final {
    while (!_threads.IsEmpty()) {
      auto thread_ptr = _threads.PopFront();
      _factory->Release(executor::IThreadPtr{thread_ptr});
    }
  }

 private:
  void Loop() {
    while (true) {
      std::unique_lock guard{_m};
      while (_tasks.IsEmpty()) {
        if (_stopped) {
          return;
        }
        _cv.wait(guard);
      }
      auto current = ITaskPtr{_tasks.PopFront()};
      guard.unlock();
      current->Call();
    }
  }

  container::intrusive::List<IThread> _threads;
  container::intrusive::List<ITask> _tasks;
  IThreadFactoryPtr _factory;
  size_t _cache_threads;
  size_t _max_threads;

  std::condition_variable _cv;
  std::mutex _m;
  bool _stopped{false};
};

}  // namespace

IThreadPool* CurrentThreadPool() {
  return tlCurrentThreadPool;
}

IThreadPoolPtr MakeThreadPool(size_t threads) {
  return std::make_shared<ThreadPool>(threads);
}

IThreadPoolPtr MakeThreadPool(IThreadFactoryPtr factory, size_t cache_threads,
                              size_t max_threads) {
  return std::make_shared<ThreadPool>(std::move(factory), cache_threads,
                                      max_threads);
}

}  // namespace yaclib::executor
