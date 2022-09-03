#pragma once

#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/thread_factory.hpp>

#include <cstddef>
#include <yaclib_std/thread>

namespace yaclib {

/**
 * Interface for thread-pool-like executors
 */
class IThreadPool : public IExecutor {
 public:
  /**
   * Wait until all threads joined or idle
   *
   * \note It is blocking
   * \note It can not be called from this thread pool job
   */
  virtual void Wait() noexcept = 0;

  /**
   * Disable further Submit() calls from being accepted
   *
   * \note after Stop() was called Alive() returns false
   */
  virtual void Stop() noexcept = 0;

  /**
   * Call Stop() and Drop() waiting tasks
   *
   * \note Drop() can be called here or in thread pool's threads
   */
  virtual void Cancel() noexcept = 0;
};

using IThreadPoolPtr = IntrusivePtr<IThreadPool>;

/**
 * TODO(kononovk): description
 * \return thread_local pointer to the thread pool that owns the current thread
 *         Inline executor if no thread pool owns the thread
 */
IExecutor& CurrentThreadPool() noexcept;

/**
 * Change thread_local pointer to executor
 */
void SetCurrentThreadPool(IExecutor& executor) noexcept;

/**
 * TODO(kononovk, MBkkt): think about ThreadPoolGuard
 */
//struct ThreadPoolGuard {
//  IThreadPoolPtr self;
//  ~ThreadPoolGuard() {
//    self->Cancel();
//    self->Wait();
//  }
//};

/**
 * Create new thread pool object
 *
 * TODO(kononovk): add description about logic of choosing thread pool's type
 *
 * \param threads the number of threads to create for this thread pool
 * \param tf thread factory to use for thread creation. \see IThreadFactory
 * \return intrusive pointer to the new ThreadPool
 */
IThreadPoolPtr MakeThreadPool(std::size_t threads = yaclib_std::thread::hardware_concurrency(),
                              IThreadFactoryPtr tf = MakeThreadFactory());

/**
 * TODO(kononovk): description
 */
IThreadPoolPtr MakeThreadPool(std::size_t threads, IThreadFactoryPtr tf, IExecutor::Type type);

}  // namespace yaclib
