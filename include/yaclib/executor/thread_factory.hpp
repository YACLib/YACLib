#pragma once

#include <yaclib/container/intrusive_node.hpp>

#include <cstddef>
#include <functional>
#include <memory>

namespace yaclib::executor {

class IThread : public container::intrusive::detail::Node<IThread> {
 public:
  virtual void Join() = 0;

  virtual ~IThread() = default;
};

using IThreadPtr = std::unique_ptr<IThread>;

class IThreadFactory {
 public:
  virtual IThreadPtr Acquire(const std::function<void()>& f) = 0;

  virtual void Release(IThreadPtr) = 0;
};

using IThreadFactoryPtr = std::shared_ptr<IThreadFactory>;

IThreadFactoryPtr CreateThreadFactory(size_t cached_threads_count,
                                      size_t max_threads_count);

}  // namespace yaclib::executor
