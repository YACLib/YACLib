#include <util/intrusive_list.hpp>

#include <yaclib/executor/thread_factory.hpp>

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>

namespace yaclib {
namespace {

void CallFunc(size_t priority, std::string_view name, util::IFunc& f, util::IFunc* acquire, util::IFunc* release) {
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
  LightThread(size_t priority, std::string_view name, util::IFuncPtr func, util::IFuncPtr acquire,
              util::IFuncPtr release)
      : _thread{[priority, name, func = std::move(func), acquire = std::move(acquire), release = std::move(release)] {
          CallFunc(priority, name, *func, acquire.Get(), release.Get());
        }} {
  }

  ~LightThread() final {
    assert(_thread.get_id() != std::this_thread::get_id());
    _thread.join();
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

  void Set(size_t priority, std::string_view name, util::IFuncPtr func, util::IFuncPtr acquire,
           util::IFuncPtr release) {
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

  ~HeavyThread() final {
    {
      std::lock_guard guard{_m};
      _state = State::Stop;
    }
    _cv.notify_all();
    assert(_thread.get_id() != std::this_thread::get_id());
    _thread.join();
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
        CallFunc(priority, name, *func, acquire.Get(), release.Get());
        guard.lock();
      }
      if (_state == State::Stop) {
        return;
      }
      _state = State::Idle;
      _cv.notify_all();
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
  util::IFuncPtr _func;
  util::IFuncPtr _acquire;
  util::IFuncPtr _release;

  std::thread _thread;
};

class ThreadFactory;

class BaseFactory : public IThreadFactory {
 public:
  virtual ThreadFactory& GetThreadFactory() = 0;
  virtual size_t GetPriority() const = 0;
  virtual std::string_view GetName() const = 0;
  virtual util::IFuncPtr GetAcquire() const = 0;
  virtual util::IFuncPtr GetRelease() const = 0;
};

class ThreadFactory : public BaseFactory {
 public:
  explicit ThreadFactory() = default;

  virtual IThreadPtr Acquire(util::IFuncPtr func, size_t priority, std::string_view name, util::IFuncPtr acquire,
                             util::IFuncPtr release) = 0;

 private:
  IThreadPtr Acquire(util::IFuncPtr func) final {
    return Acquire(std::move(func), GetPriority(), GetName(), GetAcquire(), GetRelease());
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
  util::IFuncPtr GetAcquire() const final {
    return nullptr;
  }
  util::IFuncPtr GetRelease() const final {
    return nullptr;
  }
};

class LightThreadFactory final : public ThreadFactory {
 private:
  IThreadPtr Acquire(util::IFuncPtr func, size_t priority, std::string_view name, util::IFuncPtr acquire,
                     util::IFuncPtr release) final {
    return new LightThread{priority, name, std::move(func), std::move(acquire), std::move(release)};
  }

  void Release(IThreadPtr thread) final {
    delete thread;
  }

  void IncRef() noexcept final {
  }
  void DecRef() noexcept final {
  }
};

class HeavyThreadFactory : public ThreadFactory {
 public:
  explicit HeavyThreadFactory(size_t cache_threads) : _cache_threads{cache_threads} {
  }

  ~HeavyThreadFactory() {
    while (auto thread = _threads.PopFront()) {
      delete thread;
    }
  }

 private:
  IThreadPtr Acquire(util::IFuncPtr func, size_t priority, std::string_view name, util::IFuncPtr acquire,
                     util::IFuncPtr release) final {
    std::unique_lock guard{_m};
    IThreadPtr t{_threads.PopBack()};
    if (t == nullptr) {
      t = new HeavyThread{};
      ++_threads_count;
    }
    guard.unlock();

    static_cast<HeavyThread&>(*t).Set(priority, name, std::move(func), std::move(acquire), std::move(release));
    return t;
  }

  void Release(IThreadPtr t) final {
    std::unique_lock guard{_m};
    if (_threads_count <= _cache_threads) {
      static_cast<HeavyThread&>(*t).Wait();
      _threads.PushBack(t);
    } else {
      delete t;
      --_threads_count;
    }
  }

  const size_t _cache_threads;

  std::mutex _m;
  size_t _threads_count{0};
  util::List<IThread> _threads;
};

class DecoratorThreadFactory : public BaseFactory {
 protected:
  explicit DecoratorThreadFactory(IThreadFactoryPtr base) : _base{std::move(base)} {
  }

  IThreadPtr Acquire(util::IFuncPtr func) final {
    return GetThreadFactory().Acquire(std::move(func), GetPriority(), GetName(), GetAcquire(), GetRelease());
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
  util::IFuncPtr GetAcquire() const override {
    return GetBase().GetAcquire();
  }
  util::IFuncPtr GetRelease() const override {
    return GetBase().GetRelease();
  }

 private:
  IThreadFactoryPtr _base;
};

class PriorityThreadFactory : public DecoratorThreadFactory {
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

class NamedThreadFactory : public DecoratorThreadFactory {
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

class AcquireThreadFactory : public DecoratorThreadFactory {
 public:
  AcquireThreadFactory(IThreadFactoryPtr base, util::IFuncPtr acquire)
      : DecoratorThreadFactory{std::move(base)}, _acquire{std::move(acquire)} {
  }

 private:
  util::IFuncPtr GetAcquire() const final {
    return _acquire;
  }

  util::IFuncPtr _acquire;
};

class ReleaseThreadFactory : public DecoratorThreadFactory {
 public:
  ReleaseThreadFactory(IThreadFactoryPtr base, util::IFuncPtr release)
      : DecoratorThreadFactory{std::move(base)}, _release{std::move(release)} {
  }

 private:
  util::IFuncPtr GetRelease() const final {
    return _release;
  }

  util::IFuncPtr _release;
};

class CallbackThreadFactory : public DecoratorThreadFactory {
 public:
  CallbackThreadFactory(IThreadFactoryPtr base, util::IFuncPtr acquire, util::IFuncPtr release)
      : DecoratorThreadFactory{std::move(base)}, _acquire{std::move(acquire)}, _release{std::move(release)} {
  }

 private:
  util::IFuncPtr GetAcquire() const final {
    return _acquire;
  }
  util::IFuncPtr GetRelease() const final {
    return _release;
  }

  util::IFuncPtr _acquire;
  util::IFuncPtr _release;
};

}  // namespace

IThreadFactoryPtr MakeThreadFactory(size_t cache_threads) {
  if (cache_threads == 0) {
    static LightThreadFactory sFactory;
    return IThreadFactoryPtr{&sFactory};
  }
  return new util::Counter<HeavyThreadFactory>{cache_threads};
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, size_t priority) {
  return new util::Counter<PriorityThreadFactory>(std::move(base), priority);
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, std::string name) {
  return new util::Counter<NamedThreadFactory>(std::move(base), std::move(name));
}

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, util::IFuncPtr acquire, util::IFuncPtr release) {
  if (acquire && release) {
    return new util::Counter<CallbackThreadFactory>(std::move(base), std::move(acquire), std::move(release));
  } else if (acquire) {
    return new util::Counter<AcquireThreadFactory>(std::move(base), std::move(release));
  } else if (release) {
    return new util::Counter<ReleaseThreadFactory>(std::move(base), std::move(release));
  }
  return base;  // Copy elision doesn't work for function arguments, but implicit move guaranteed by standard
}

}  // namespace yaclib
