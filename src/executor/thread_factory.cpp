#include <yaclib/container/intrusive_node.hpp>
#include <yaclib/executor/thread_factory.hpp>

#include <atomic>
#include <thread>

namespace yaclib::executor {

namespace {

class Thread final : public IThread {
 public:
  explicit Thread(const std::function<void()>& f) : _thread{f} {
  }

  void Join() final {
    if (_thread.joinable()) {
      _thread.join();
    }
  }

 private:
  std::thread _thread;
};

class ThreadFactory final : public IThreadFactory {
 public:
  ThreadFactory(size_t cached_threads_count, size_t max_threads_count)
      : _cached_threads_count{cached_threads_count},
        _max_threads_count{max_threads_count} {
  }

 private:
  IThreadPtr Acquire(const std::function<void()>& f) final {
    if (_threads_count.fetch_add(1, std::memory_order_acq_rel) <
        _max_threads_count) {
      return std::make_unique<Thread>(f);
    }

    _threads_count.fetch_sub(1, std::memory_order_relaxed);
    return nullptr;
  }

  void Release(IThreadPtr thread) final {
    _threads_count.fetch_sub(1, std::memory_order_relaxed);
    thread.reset();
  }

  std::atomic<size_t> _threads_count{0};
  const size_t _cached_threads_count;
  const size_t _max_threads_count;
};

}  // namespace

IThreadFactoryPtr CreateThreadFactory(size_t cached_threads_count,
                                      size_t max_threads_count) {
  return std::make_shared<ThreadFactory>(cached_threads_count,
                                         max_threads_count);
}

}  // namespace yaclib::executor
