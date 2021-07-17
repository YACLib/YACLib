#pragma once

#include <yaclib/container/intrusive_node.hpp>
#include <yaclib/task.hpp>

#include <cstddef>
#include <functional>
#include <memory>

namespace yaclib::executor {

class IThread : public container::intrusive::detail::Node<IThread> {
 public:
  virtual ~IThread() = default;
};

using IThreadPtr = std::unique_ptr<IThread>;

class IThreadFactory {
 public:
  virtual IThreadPtr Acquire(Functor functor) = 0;

  virtual void Release(IThreadPtr thread) = 0;
};

using IThreadFactoryPtr = std::shared_ptr<IThreadFactory>;

IThreadFactoryPtr MakeThreadFactory(size_t cache_threads, size_t max_threads);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, std::string name);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, size_t priority);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, Functor acquire,
                                    Functor release);

}  // namespace yaclib::executor
