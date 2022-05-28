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
   * Wait until task counter is zero and call Stop(). Check \ref Stop for details
   */
  virtual void SoftStop() = 0;

  /**
   * Disable further Submit() calls from being accepted
   */
  virtual void Stop() = 0;

  /**
   * Call Stop() and Drop() waiting tasks
   */
  virtual void HardStop() = 0;

  /**
   * Wait until all threads join
   *
   * \note This method is blocking.
   * Note that in order for an IThreadPool to join all the threads, one Stop methods must be called.
   * \see SoftStop, Stop, HardStop
   */
  virtual void Wait() = 0;
};

using IThreadPoolPtr = IntrusivePtr<IThreadPool>;

/**
 * \return Thread local pointer to the ThreadPool that owns the current thread
 *         Inline executor if no ThreadPool owns the thread
 */
IExecutor& CurrentThreadPool() noexcept;

/**
 *
 */
void SetCurrentThreadPool(IExecutor& executor) noexcept;

/**
 * Create new ThreadPool object
 *
 * \param threads the number of threads to create for this ThreadPool
 * \param tf thread factory to use for thread creation. \see IThreadFactory
 * \return intrusive pointer to the new ThreadPool
 */
IThreadPoolPtr MakeThreadPool(std::size_t threads = std::thread::hardware_concurrency(),
                              IThreadFactoryPtr tf = MakeThreadFactory());

}  // namespace yaclib
