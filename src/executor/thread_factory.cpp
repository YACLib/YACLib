#include <container/intrusive_list.hpp>

#include <yaclib/executor/thread_factory.hpp>

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>

namespace yaclib::executor {
namespace {

void CallFunc(size_t priority, std::string_view name, IFunc& func,
              IFunc* acquire, IFunc* release) {
  if (acquire) {
    acquire->Call();
  }
  // TODO(MBkkt) set priority
  // TODO(MBkkt) set name
  func.Call();
  if (release) {
    release->Call();
  }
}

class LightThread final : public IThread {
 public:
  LightThread(size_t priority, std::string_view name, IFuncPtr func,
              IFuncPtr acquire, IFuncPtr release)
      : _thread{[priority, name, func = std::move(func),
                 acquire = std::move(acquire), release = std::move(release)] {
          CallFunc(priority, name, *func, acquire.get(), release.get());
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

  void Set(size_t priority, std::string_view name, IFuncPtr func,
           IFuncPtr acquire, IFuncPtr release) {
    {
      std::lock_guard guard{_m};
      assert(_state == State::Idle);
      _state = State::Run;
      _priority = priority;
      _name = name;
      _func = std::move(func);
      _acquire = std::move(acquire);
      _release = std::move(release);
    }
    _cv.notify_all();
  }

  void Wait() {
    std::unique_lock guard{_m};
    while (_state == State::Run) {
      _cv.wait(guard);
    }
  }

  void Join() {
    {
      std::lock_guard guard{_m};
      _state = State::Stop;
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
      while (_func) {
        auto priority = std::exchange(_priority, 0);
        auto name = std::exchange(_name, "");
        auto func = std::exchange(_func, nullptr);
        auto acquire = std::exchange(_acquire, nullptr);
        auto release = std::exchange(_release, nullptr);

        guard.unlock();
        CallFunc(priority, name, *func, acquire.get(), release.get());
        guard.lock();
      }
      if (_state == State::Stop) {
        return;
      } else {
        _state = State::Idle;
        _cv.notify_one();
      }
      _cv.wait(guard);
    }
  }

  std::mutex _m;
  std::condition_variable _cv;
  enum class State {
    Idle,
    Run,
    Stop,
  };
  State _state{State::Idle};

  size_t _priority{0};
  std::string_view _name;
  IFuncPtr _func;
  IFuncPtr _acquire;
  IFuncPtr _release;

  std::thread _thread;
};

class ThreadFactory;

class BaseFactory : public IThreadFactory {
 public:
  virtual ThreadFactory& GetThreadFactory() = 0;
  virtual size_t GetPriority() const = 0;
  virtual std::string_view GetName() const = 0;
  virtual IFuncPtr GetAcquire() const = 0;
  virtual IFuncPtr GetRelease() const = 0;
};

class ThreadFactory : public BaseFactory {
 public:
  explicit ThreadFactory() = default;

  virtual IThreadPtr Acquire(IFuncPtr func, size_t priority,
                             std::string_view name, IFuncPtr acquire,
                             IFuncPtr release) = 0;

 private:
  IThreadPtr Acquire(IFuncPtr func) final {
    return Acquire(std::move(func), GetPriority(), GetName(), GetAcquire(),
                   GetRelease());
  }

  ThreadFactory& GetThreadFactory() final {
    return *this;
  }
  size_t GetPriority() const final {
    return 0;
  }
  std::string_view GetName() const final {
    return "";
  }
  IFuncPtr GetAcquire() const final {
    return nullptr;
  }
  IFuncPtr GetRelease() const final {
    return nullptr;
  }
};

class LightThreadFactory final : public ThreadFactory {
 public:
  LightThreadFactory() = default;

 private:
  IThreadPtr Acquire(IFuncPtr func, size_t priority, std::string_view name,
                     IFuncPtr acquire, IFuncPtr release) final {
    return std::make_unique<LightThread>(priority, name, std::move(func),
                                         std::move(acquire),
                                         std::move(release));
  }

  void Release(IThreadPtr thread) final {
    static_cast<LightThread&>(*thread).Join();
    thread.reset();
  }
};

class HeavyThreadFactory final : public ThreadFactory {
 public:
  explicit HeavyThreadFactory(size_t cache_threads)
      : _cache_threads{cache_threads} {
  }

 private:
  IThreadPtr Acquire(IFuncPtr func, size_t priority, std::string_view name,
                     IFuncPtr acquire, IFuncPtr release) final {
    std::unique_lock guard{_m};
    IThreadPtr thread{_threads.PopBack()};
    if (thread == nullptr) {
      thread = std::make_unique<HeavyThread>();
      ++_threads_count;
    }
    guard.unlock();

    static_cast<HeavyThread&>(*thread).Set(priority, name, std::move(func),
                                           std::move(acquire),
                                           std::move(release));
    return thread;
  }

  void Release(IThreadPtr thread) final {
    std::unique_lock guard{_m};
    if (_threads_count <= _cache_threads) {
      static_cast<HeavyThread&>(*thread).Wait();
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

  IThreadPtr Acquire(IFuncPtr func) final {
    return GetThreadFactory().Acquire(std::move(func), GetPriority(), GetName(),
                                      GetAcquire(), GetRelease());
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
  IFuncPtr GetAcquire() const override {
    return GetBase().GetAcquire();
  }
  IFuncPtr GetRelease() const override {
    return GetBase().GetRelease();
  }

 private:
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
  AcquireThreadFactory(IThreadFactoryPtr base, IFuncPtr acquire)
      : DecoratorThreadFactory{std::move(base)}, _acquire{std::move(acquire)} {
  }

 private:
  IFuncPtr GetAcquire() const final {
    return _acquire;
  }

  IFuncPtr _acquire;
};

class ReleaseThreadFactory final : public DecoratorThreadFactory {
 public:
  ReleaseThreadFactory(IThreadFactoryPtr base, IFuncPtr release)
      : DecoratorThreadFactory{std::move(base)}, _release{std::move(release)} {
  }

 private:
  IFuncPtr GetRelease() const final {
    return _release;
  }

  IFuncPtr _release;
};

class CallbackThreadFactory final : public DecoratorThreadFactory {
 public:
  CallbackThreadFactory(IThreadFactoryPtr base, IFuncPtr acquire,
                        IFuncPtr release)
      : DecoratorThreadFactory{std::move(base)},
        _acquire{std::move(acquire)},
        _release{std::move(release)} {
  }

 private:
  IFuncPtr GetAcquire() const final {
    return _acquire;
  }
  IFuncPtr GetRelease() const final {
    return _release;
  }

  IFuncPtr _acquire;
  IFuncPtr _release;
};

}  // namespace

IThreadFactoryPtr MakeThreadFactory(size_t cache_threads) {
  if (cache_threads == 0) {
    static auto factory = std::make_shared<LightThreadFactory>();
    return factory;
  }
  return std::make_shared<HeavyThreadFactory>(cache_threads);
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, size_t priority) {
  return std::make_shared<PriorityThreadFactory>(std::move(base), priority);
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, std::string name) {
  return std::make_shared<NamedThreadFactory>(std::move(base), std::move(name));
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, IFuncPtr acquire,
                                    IFuncPtr release) {
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
