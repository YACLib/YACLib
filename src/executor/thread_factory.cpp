#include <container/intrusive_list.hpp>

#include <yaclib/container/intrusive_node.hpp>
#include <yaclib/executor/thread_factory.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>

namespace yaclib::executor {
namespace {

void CallFunctor(size_t priority, std::string_view name, ITask& functor,
                 ITask* acquire, ITask* release) {
  if (acquire) {
    acquire->Call();
  }
  // TODO(MBkkt) set priority
  // TODO(MBkkt) set name
  functor.Call();
  if (release) {
    release->Call();
  }
}

class LightThread final : public IThread {
 public:
  LightThread(size_t priority, std::string_view name, Functor functor,
              Functor acquire, Functor release)
      : _thread{[priority, name, functor = std::move(functor),
                 acquire = std::move(acquire), release = std::move(release)] {
          CallFunctor(priority, name, *functor, acquire.get(), release.get());
        }} {
  }

  void Join() {
    if (_thread.joinable()) {
      _thread.join();
    }
  }

 private:
  std::thread _thread;
};

class HeavyThread final : public IThread {
 public:
  explicit HeavyThread()
      : _thread{[this] {
          Loop();
        }} {
  }

  void Set(size_t priority, std::string_view name, Functor functor,
           Functor acquire, Functor release) {
    {
      std::lock_guard guard{_m};
      _priority = priority;
      _name = name;
      _functor = std::move(functor);
      _acquire = std::move(acquire);
      _release = std::move(release);
    }
    _cv.notify_all();
  }

  void Join() {
    {
      std::lock_guard guard{_m};
      _stopped = true;
    }
    _cv.notify_all();
    if (_thread.joinable()) {
      _thread.join();
    }
  }

 private:
  void Loop() {
    std::unique_lock guard{_m};
    while (true) {
      while (!_stopped && !_functor) {
        _cv.wait(guard);
      }
      if (!_functor) {
        return;
      }

      auto priority = std::exchange(_priority, 0);
      auto name = std::exchange(_name, "");
      auto functor = std::exchange(_functor, nullptr);
      auto acquire = std::exchange(_acquire, nullptr);
      auto release = std::exchange(_release, nullptr);

      guard.unlock();
      CallFunctor(priority, name, *functor, acquire.get(), release.get());
      guard.lock();
    }
  }

  std::mutex _m;
  std::condition_variable _cv;
  bool _stopped{false};

  size_t _priority{0};
  std::string_view _name;
  Functor _functor;
  Functor _acquire;
  Functor _release;

  std::thread _thread;
};

class ThreadFactory;

class BaseFactory : public IThreadFactory {
 public:
  virtual ThreadFactory& GetThreadFactory() = 0;
  virtual size_t GetPriority() const = 0;
  virtual std::string_view GetName() const = 0;
  virtual Functor GetAcquire() const = 0;
  virtual Functor GetRelease() const = 0;
};

class ThreadFactory : public BaseFactory {
 public:
  explicit ThreadFactory(size_t max_threads) : _max_threads{max_threads} {
  }

  virtual IThreadPtr Acquire(Functor functor, size_t priority,
                             std::string_view name, Functor acquire,
                             Functor release) = 0;

 protected:
  const size_t _max_threads;

 private:
  IThreadPtr Acquire(Functor functor) final {
    return Acquire(std::move(functor), GetPriority(), GetName(), GetAcquire(),
                   GetRelease());
  }

  ThreadFactory& GetThreadFactory() override {
    return *this;
  }
  size_t GetPriority() const override {
    return 0;
  }
  std::string_view GetName() const override {
    return "";
  }
  Functor GetAcquire() const override {
    return nullptr;
  }
  Functor GetRelease() const override {
    return nullptr;
  }
};

class LightThreadFactory final : public ThreadFactory {
 public:
  explicit LightThreadFactory(size_t max_threads) : ThreadFactory{max_threads} {
  }

 private:
  IThreadPtr Acquire(Functor functor, size_t priority, std::string_view name,
                     Functor acquire, Functor release) final {
    if (_threads.fetch_add(1, std::memory_order_acq_rel) < _max_threads) {
      return std::make_unique<LightThread>(priority, name, std::move(functor),
                                           std::move(acquire),
                                           std::move(release));
    }
    // It's fast and ensures that we never create more than _max_threads
    // std::thread, but it doesn't guarantee that we won't get nullptr back
    // when _threads < _max_threads.
    _threads.fetch_sub(1, std::memory_order_relaxed);
    return nullptr;
  }

  void Release(IThreadPtr thread) final {
    static_cast<LightThread&>(*thread).Join();
    thread.reset();
    _threads.fetch_sub(1, std::memory_order_release);
  }

  std::atomic_size_t _threads{0};
};

class HeavyThreadFactory final : public ThreadFactory {
 public:
  HeavyThreadFactory(size_t cache_threads, size_t max_threads)
      : ThreadFactory{max_threads}, _cache_threads{cache_threads} {
  }

 private:
  IThreadPtr Acquire(Functor functor, size_t priority, std::string_view name,
                     Functor acquire, Functor release) final {
    std::unique_lock guard{_m};
    if (_threads_count == _max_threads) {
      return nullptr;
    }
    IThreadPtr thread{_threads.PopBack()};
    if (thread == nullptr) {
      thread = std::make_unique<HeavyThread>();
      ++_threads_count;
    }
    guard.unlock();

    static_cast<HeavyThread&>(*thread).Set(priority, name, std::move(functor),
                                           std::move(acquire),
                                           std::move(release));
    return thread;
  }

  void Release(IThreadPtr thread) final {
    std::unique_lock guard{_m};
    if (_threads_count < _cache_threads) {
      _threads.PushBack(thread.release());
    } else {
      static_cast<HeavyThread&>(*thread).Join();
      thread.reset();
      --_threads_count;
    }
  }

  const size_t _cache_threads;

  std::mutex _m;
  size_t _threads_count{0};
  container::intrusive::List<IThread> _threads;
};

class DecoratorThreadFactory : public BaseFactory {
 protected:
  explicit DecoratorThreadFactory(IThreadFactoryPtr base)
      : _base{std::move(base)} {
  }

  IThreadPtr Acquire(Functor functor) final {
    return GetThreadFactory().Acquire(std::move(functor), GetPriority(),
                                      GetName(), GetAcquire(), GetRelease());
  }
  void Release(IThreadPtr thread) final {
    _base->Release(std::move(thread));
  }

  const BaseFactory& GetBase() const {
    return static_cast<const BaseFactory&>(*_base);
  }

  ThreadFactory& GetThreadFactory() final {
    return static_cast<BaseFactory&>(*_base).GetThreadFactory();
  }
  size_t GetPriority() const override {
    return GetBase().GetPriority();
  }
  std::string_view GetName() const override {
    return GetBase().GetName();
  }
  Functor GetAcquire() const override {
    return GetBase().GetAcquire();
  }
  Functor GetRelease() const override {
    return GetBase().GetRelease();
  }

  IThreadFactoryPtr _base;
};

class PriorityThreadFactory final : public DecoratorThreadFactory {
 public:
  PriorityThreadFactory(IThreadFactoryPtr base, size_t priority)
      : DecoratorThreadFactory{std::move(base)}, _priority{priority} {
  }

 private:
  size_t GetPriority() const final {
    return _priority;
  }

  size_t _priority;
};

class NamedThreadFactory final : public DecoratorThreadFactory {
 public:
  NamedThreadFactory(IThreadFactoryPtr base, std::string name)
      : DecoratorThreadFactory{std::move(base)}, _name{std::move(name)} {
  }

 private:
  std::string_view GetName() const final {
    return _name;
  }

  std::string _name;
};

class AcquireThreadFactory final : public DecoratorThreadFactory {
 public:
  AcquireThreadFactory(IThreadFactoryPtr base, Functor acquire)
      : DecoratorThreadFactory{std::move(base)}, _acquire{std::move(acquire)} {
  }

 private:
  Functor GetAcquire() const final {
    return _acquire;
  }

  Functor _acquire;
};

class ReleaseThreadFactory final : public DecoratorThreadFactory {
 public:
  ReleaseThreadFactory(IThreadFactoryPtr base, Functor release)
      : DecoratorThreadFactory{std::move(base)}, _release{std::move(release)} {
  }

 private:
  Functor GetRelease() const final {
    return _release;
  }

  Functor _release;
};

class CallbackThreadFactory final : public DecoratorThreadFactory {
 public:
  CallbackThreadFactory(IThreadFactoryPtr base, Functor acquire,
                        Functor release)
      : DecoratorThreadFactory{std::move(base)},
        _acquire{std::move(acquire)},
        _release{std::move(release)} {
  }

 private:
  Functor GetAcquire() const final {
    return _acquire;
  }
  Functor GetRelease() const final {
    return _release;
  }

  Functor _acquire;
  Functor _release;
};

}  // namespace

IThreadFactoryPtr MakeThreadFactory(size_t cache_threads, size_t max_threads) {
  if (cache_threads == 0) {
    return std::make_shared<LightThreadFactory>(max_threads);
  }
  return std::make_shared<HeavyThreadFactory>(cache_threads, max_threads);
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, size_t priority) {
  return std::make_shared<PriorityThreadFactory>(std::move(base), priority);
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, std::string name) {
  return std::make_shared<NamedThreadFactory>(std::move(base), std::move(name));
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, Functor acquire,
                                    Functor release) {
  if (acquire && release) {
    return std::make_shared<CallbackThreadFactory>(
        std::move(base), std::move(acquire), std::move(release));
  } else if (acquire) {
    return std::make_shared<AcquireThreadFactory>(std::move(base),
                                                  std::move(release));
  } else if (release) {
    return std::make_shared<ReleaseThreadFactory>(std::move(base),
                                                  std::move(release));
  }
  return base;
}

}  // namespace yaclib::executor
