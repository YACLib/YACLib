#pragma once

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/thread_factory.hpp>

#include <memory>
#include <thread>

namespace yaclib::executor {

class IThreadPool : public IExecutor {
 public:
  virtual void SoftStop() = 0;

  virtual void Stop() = 0;

  virtual void HardStop() = 0;

  virtual void Wait() = 0;
};

using IThreadPoolPtr = container::intrusive::Ptr<IThreadPool>;

IThreadPool* CurrentThreadPool() noexcept;

IThreadPoolPtr MakeThreadPool(
    size_t threads = std::thread::hardware_concurrency(),
    IThreadFactoryPtr factory = MakeThreadFactory());

}  // namespace yaclib::executor
