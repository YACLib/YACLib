#pragma once

#include <yaclib/container/intrusive_node.hpp>
#include <yaclib/task.hpp>

#include <cstddef>
#include <functional>
#include <memory>

namespace yaclib::executor {

class IThread : public container::intrusive::detail::Node {
 public:
  virtual ~IThread() = default;
};

using IThreadPtr = std::unique_ptr<IThread>;

class IThreadFactory {
 public:
  virtual IThreadPtr Acquire(IFuncPtr functor) = 0;

  virtual void Release(IThreadPtr thread) = 0;

  virtual ~IThreadFactory() = default;
};

using IThreadFactoryPtr = std::shared_ptr<IThreadFactory>;

IThreadFactoryPtr MakeThreadFactory();

IThreadFactoryPtr MakeThreadFactory(size_t cache_threads, size_t max_threads);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, std::string name);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, size_t priority);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, IFuncPtr acquire,
                                    IFuncPtr release);

}  // namespace yaclib::executor
