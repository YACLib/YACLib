#include <util/intrusive_list.hpp>

#include <yaclib/executor/thread_factory.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <yaclib_std/condition_variable>
#include <yaclib_std/mutex>
#include <yaclib_std/thread>

namespace yaclib {
namespace {

void CallFunc(std::size_t /*priority*/, std::string_view /*name*/, IFunc& f, IFunc* acquire, IFunc* release) {
  if (acquire != nullptr) {
    acquire->Call();
  }
  // TODO(MBkkt) set priority
  // TODO(MBkkt) set name
  f.Call();
  if (release != nullptr) {
    release->Call();
  }
}

class LightThread final : public IThread {
 public:
  LightThread(std::size_t priority, std::string_view name, IFuncPtr func, IFuncPtr acquire, IFuncPtr release)
      : _thread{[priority, name, func = std::move(func), acquire = std::move(acquire), release = std::move(release)] {
          CallFunc(priority, name, *func, acquire.Get(), release.Get());
        }} {
  }

  ~LightThread() final {
    YACLIB_ERROR(_thread.get_id() == yaclib_std::this_thread::get_id(),
                 "Thread try to join itself, probably because you forgot Stop ThreadPool, "
                 "and ThreadPool dtor was called in ThreadPool thread");
    _thread.join();
  }

 private:
  yaclib_std::thread _thread;
};

class HeavyThread final : public IThread {
 public:
  explicit HeavyThread()
      : _thread{[this] {
          Loop();
        }} {
  }

  void Set(std::size_t priority, std::string_view name, IFuncPtr func, IFuncPtr acquire, IFuncPtr release) {
    {
      std::lock_guard lock{_m};
      YACLIB_ERROR(_state != State::Idle, "Trying run some func on not idle thread");
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
    std::unique_lock lock{_m};
    while (_state == State::Run) {
      _cv.wait(lock);
    }
  }

  ~HeavyThread() final {
    YACLIB_ERROR(_thread.get_id() == yaclib_std::this_thread::get_id(),
                 "Thread try to join itself, probably because you forgot Stop ThreadPool, "
                 "and ThreadPool dtor was called in ThreadPool thread");
    {
      std::lock_guard lock{_m};
      _state = State::Stop;
    }
    _cv.notify_all();
    _thread.join();
  }

 private:
  void Loop() {
    std::unique_lock lock{_m};
    while (true) {
      while (_func) {
        auto priority = std::exchange(_priority, 0);
        auto name = std::exchange(_name, "");
        auto func = std::exchange(_func, nullptr);
        auto acquire = std::exchange(_acquire, nullptr);
        auto release = std::exchange(_release, nullptr);

        lock.unlock();
        CallFunc(priority, name, *func, acquire.Get(), release.Get());
        lock.lock();
      }
      if (_state == State::Stop) {
        return;
      }
      _state = State::Idle;
      _cv.notify_all();
      _cv.wait(lock);
    }
  }

  yaclib_std::mutex _m;
  yaclib_std::condition_variable _cv;
  enum class State {
    Idle,
    Run,
    Stop,
  };
  State _state{State::Idle};

  std::size_t _priority{0};
  std::string_view _name;
  IFuncPtr _func;
  IFuncPtr _acquire;
  IFuncPtr _release;

  yaclib_std::thread _thread;
};

class ThreadFactory;

class BaseFactory : public IThreadFactory {
 public:
  virtual ThreadFactory& GetThreadFactory() = 0;
  virtual std::size_t GetPriority() const = 0;
  virtual std::string_view GetName() const = 0;
  virtual IFuncPtr GetAcquire() const = 0;
  virtual IFuncPtr GetRelease() const = 0;
};

class ThreadFactory : public BaseFactory {
 public:
  explicit ThreadFactory() = default;

  virtual IThread* Acquire(IFuncPtr func, std::size_t priority, std::string_view name, IFuncPtr acquire,
                           IFuncPtr release) = 0;

 private:
  IThread* Acquire(IFuncPtr func) final {
    return Acquire(std::move(func), GetPriority(), GetName(), GetAcquire(), GetRelease());
  }

  ThreadFactory& GetThreadFactory() final {
    return *this;
  }
  std::size_t GetPriority() const final {
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
 private:
  IThread* Acquire(IFuncPtr func, std::size_t priority, std::string_view name, IFuncPtr acquire,
                   IFuncPtr release) final {
    return new LightThread{priority, name, std::move(func), std::move(acquire), std::move(release)};
  }

  void Release(IThread* thread) final {
    delete thread;
  }

  void IncRef() noexcept final {
  }
  void DecRef() noexcept final {
  }
};

class HeavyThreadFactory : public ThreadFactory {
 public:
  explicit HeavyThreadFactory(std::size_t cache_threads) : _cache_threads{cache_threads} {
  }

  ~HeavyThreadFactory() override {
    while (!_threads.Empty()) {
      auto& thread = _threads.PopFront();
      delete &static_cast<IThread&>(thread);
    }
  }

 private:
  IThread* Acquire(IFuncPtr func, std::size_t priority, std::string_view name, IFuncPtr acquire,
                   IFuncPtr release) final {
    HeavyThread* thread = nullptr;
    if (std::lock_guard lock{_m}; _threads.Empty()) {
      thread = new HeavyThread{};
      ++_threads_count;
    } else {
      thread = &static_cast<HeavyThread&>(_threads.PopFront());
    }
    thread->Set(priority, name, std::move(func), std::move(acquire), std::move(release));
    return thread;
  }

  void Release(IThread* thread) final {
    std::unique_lock lock{_m};
    if (_threads_count <= _cache_threads) {
      static_cast<HeavyThread&>(*thread).Wait();
      _threads.PushFront(*thread);
    } else {
      delete thread;
      --_threads_count;
    }
  }

  const std::size_t _cache_threads;

  yaclib_std::mutex _m;
  std::size_t _threads_count{0};
  detail::List _threads;
};

class DecoratorThreadFactory : public BaseFactory {
 protected:
  explicit DecoratorThreadFactory(IThreadFactoryPtr base) : _base{std::move(base)} {
  }

  IThread* Acquire(IFuncPtr func) final {
    return GetThreadFactory().Acquire(std::move(func), GetPriority(), GetName(), GetAcquire(), GetRelease());
  }
  void Release(IThread* thread) final {
    _base->Release(thread);
  }

  const BaseFactory& GetBase() const {
    return static_cast<const BaseFactory&>(*_base);
  }

  ThreadFactory& GetThreadFactory() final {
    return static_cast<BaseFactory&>(*_base).GetThreadFactory();
  }
  std::size_t GetPriority() const override {
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

class PriorityThreadFactory : public DecoratorThreadFactory {
 public:
  PriorityThreadFactory(IThreadFactoryPtr base, std::size_t priority)
      : DecoratorThreadFactory{std::move(base)}, _priority{priority} {
  }

 private:
  std::size_t GetPriority() const final {
    return _priority;
  }

  std::size_t _priority;
};

class NamedThreadFactory : public DecoratorThreadFactory {
 public:
  NamedThreadFactory(IThreadFactoryPtr base, std::string_view name)
      : DecoratorThreadFactory{std::move(base)}, _name{name} {
  }

 private:
  [[nodiscard]] std::string_view GetName() const final {
    return _name;
  }

  std::string _name;
};

class AcquireThreadFactory : public DecoratorThreadFactory {
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

class ReleaseThreadFactory : public DecoratorThreadFactory {
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

class CallbackThreadFactory : public DecoratorThreadFactory {
 public:
  CallbackThreadFactory(IThreadFactoryPtr base, IFuncPtr acquire, IFuncPtr release)
      : DecoratorThreadFactory{std::move(base)}, _acquire{std::move(acquire)}, _release{std::move(release)} {
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

LightThreadFactory sFactory;

}  // namespace

IThreadFactoryPtr MakeThreadFactory(std::size_t cache_threads) {
  if (cache_threads == 0) {
    return IThreadFactoryPtr{&sFactory};
  }
  return MakeIntrusive<HeavyThreadFactory, IThreadFactory>(cache_threads);
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, std::size_t priority) {
  return MakeIntrusive<PriorityThreadFactory, IThreadFactory>(std::move(base), priority);
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, std::string_view name) {
  return MakeIntrusive<NamedThreadFactory, IThreadFactory>(std::move(base), name);
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, IFuncPtr acquire, IFuncPtr release) {
  if (acquire && release) {
    return MakeIntrusive<CallbackThreadFactory, IThreadFactory>(std::move(base), std::move(acquire),
                                                                std::move(release));
  } else if (acquire) {
    return MakeIntrusive<AcquireThreadFactory, IThreadFactory>(std::move(base), std::move(acquire));
  } else if (release) {
    return MakeIntrusive<ReleaseThreadFactory, IThreadFactory>(std::move(base), std::move(release));
  }
  return base;  // Copy elision doesn't work for function arguments, but implicit move guaranteed by standard
}

}  // namespace yaclib
