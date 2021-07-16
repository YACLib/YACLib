#include <container/intrusive_list.hpp>

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <condition_variable>
#include <mutex>

namespace yaclib::executor {

namespace {

class ThreadPool final : public IThreadPool {
 public:
  ThreadPool(IThreadFactoryPtr factory, size_t min_threads_count,
             size_t max_threads_count)
      : _min_threads_count{min_threads_count},
        _max_threads_count{max_threads_count} {
    auto loop_functor = [this] {
      Loop();
    };
    for (size_t i = 0; i != _min_threads_count; ++i) {
      _threads.PushBack(factory->Acquire(loop_functor).release());
    }
  }

  ~ThreadPool() final {
    // TODO(kononovk): #2 review
    Cancel();
    Wait();
  }

 private:
  void Execute(ITaskPtr task) final {
    // TODO(kononovk): #2 implement
  }

  void Stop() final {
    // TODO(kononovk): #2 After call: сalling Execute() for all,
    //  expect this ThreadPool tasks, does nothing
  }

  void Close() final {
    // TODO(kononovk): #2 After call: сalling Execute() for all, does nothing
  }

  void Cancel() final {
    // TODO(kononovk): #2 After call: сalling Execute() for all,
    //  does nothing and remove all not
  }

  void Wait() final {
    // TODO(kononovk): #2 After call: this ThreadPool haven't running threads
  }

  void Loop() {
    /* TODO(kononovk): #2 implement
    while (true) {
      std::unique_lock guard{_m};
      while (_tasks.IsEmpty()) {
        if (!_running) {
          return;
        }
        _cv.wait(guard);
      }
      auto current = _tasks.PopFront();
      guard.unlock();

      current->Call();
      delete current;
    }
     */
  }

  container::intrusive::List<IThread> _threads;
  container::intrusive::List<ITask> _tasks;
  size_t _min_threads_count;
  size_t _max_threads_count;

  std::condition_variable _cv;
  std::mutex _m;

  bool _running{true};
};

}  // namespace

IThreadPoolPtr CreateThreadPool(IThreadFactoryPtr factory,
                                size_t cached_threads_count,
                                size_t max_threads_count) {
  return std::make_shared<ThreadPool>(factory, cached_threads_count,
                                      max_threads_count);
}

}  // namespace yaclib::executor
