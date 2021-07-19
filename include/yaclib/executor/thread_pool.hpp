#pragma once

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/thread_factory.hpp>

#include <memory>

namespace yaclib::executor {

class IThreadPool : public IExecutor {
 public:
  virtual void Stop() = 0;

  virtual void Close() = 0;

  virtual void Cancel() = 0;

  virtual void Wait() = 0;
};

using IThreadPoolPtr = std::shared_ptr<IThreadPool>;

IThreadPool* CurrentThreadPool();

IThreadPoolPtr MakeThreadPool(size_t threads);

IThreadPoolPtr MakeThreadPool(IThreadFactoryPtr factory, size_t cache_threads,
                              size_t max_threads);

}  // namespace yaclib::executor
