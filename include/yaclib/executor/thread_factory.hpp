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

class IThreadFactory : public IRef {
 public:
  virtual IThreadPtr Acquire(IFuncPtr func) = 0;

  virtual void Release(IThreadPtr thread) = 0;
};

using IThreadFactoryPtr = container::intrusive::Ptr<IThreadFactory>;

IThreadFactoryPtr MakeThreadFactory(size_t cache_threads = 0);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, std::string name);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, size_t priority);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, IFuncPtr acquire,
                                    IFuncPtr release);

}  // namespace yaclib::executor
