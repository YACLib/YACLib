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

IThreadPoolPtr CreateThreadPool(IThreadFactoryPtr factory,
                                size_t cached_threads_count,
                                size_t max_threads_count);

}  // namespace yaclib::executor
