#include <util/intrusive_list.hpp>

#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/exe/thread_factory.hpp>
#include <yaclib/exe/thread_pool.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstddef>
#include <utility>
#include <yaclib_std/condition_variable>
#include <yaclib_std/mutex>
#include <yaclib_std/thread_local>

namespace yaclib {
namespace {

class FairThreadPool : public IThreadPool, private IFunc {
 public:
  explicit FairThreadPool(IThreadFactoryPtr factory) noexcept : _factory{std::move(factory)}, _task_count{0} {
  }

  void Start(std::size_t threads) {  // needed to prevent pure virtual call
    for (std::size_t i = 0; i != threads; ++i) {
      auto* thread = _factory->Acquire(this);
      YACLIB_ERROR(thread == nullptr, "Acquired from thread factory thread is null");
      _threads.PushBack(*thread);
    }
  }

  ~FairThreadPool() override {
    Cancel();
    // TODO(kononovk)
    while (!_threads.Empty()) {
      auto& thread = _threads.PopFront();
      _factory->Release(&static_cast<IThread&>(thread));
    }
  }

 private:
  [[nodiscard]] Type Tag() const noexcept final {
    return Type::FairThreadPool;
  }

  [[nodiscard]] bool Alive() const noexcept final {
    std::lock_guard lock{_m};
    return !WasStop();
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

  void Wait() noexcept final {
    std::unique_lock lock{_m};
    while (!NoTasks()) {
      _empty.wait(lock);
    }
  }

  void Stop() noexcept final {
    std::unique_lock lock{_m};
    StopImpl(std::move(lock));
  }

  void Cancel() noexcept final {
    std::unique_lock lock{_m};
    detail::List tasks{std::move(_tasks)};
    StopImpl(std::move(lock));
    while (!tasks.Empty()) {
      auto& task = tasks.PopFront();
      static_cast<Job&>(task).Drop();
    }
  }

  [[nodiscard]] bool WasStop() const noexcept {
    return (_task_count & 1U) != 0U;
  }
  [[nodiscard]] bool NoTasks() const noexcept {
    return (_task_count >> 2U) == 0;
  }

  void StopImpl(std::unique_lock<yaclib_std::mutex>&& lock) noexcept {
    _task_count |= 1U;
    lock.unlock();
    _idle.notify_all();
  }

  void Loop() noexcept {
    std::unique_lock lock{_m};
    while (true) {
      while (!_tasks.Empty()) {
        YACLIB_ASSERT(!NoTasks());
        auto& task = _tasks.PopFront();
        lock.unlock();
        static_cast<Job&>(task).Call();
        lock.lock();
        _task_count -= 4;  // Pop Task
      }
      if (NoTasks()) {
        _empty.notify_all();
      }
      if (WasStop()) {
        return;
      }
      _idle.wait(lock);
    }
  }

  void Call() noexcept final {
    SetCurrentThreadPool(*this);
    Loop();
    SetCurrentThreadPool(MakeInline());
  }

  mutable yaclib_std::mutex _m;
  yaclib_std::condition_variable _idle;
  yaclib_std::condition_variable _empty;
  IThreadFactoryPtr _factory;
  detail::List _threads;
  detail::List _tasks;
  std::size_t _task_count;
};

}  // namespace

IThreadPoolPtr MakeFairThreadPool(std::size_t threads, IThreadFactoryPtr tf) {
  auto tp = MakeShared<FairThreadPool>(1, std::move(tf));
  tp->Start(threads);
  return tp;
}

}  // namespace yaclib
