#pragma once

#include <yaclib/config.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/util/detail/node.hpp>
#include <yaclib/util/detail/shared_func.hpp>

#include <cstddef>
#include <string_view>

namespace yaclib {

class IThread : public detail::Node {
 public:
  virtual ~IThread() = default;
};

using IThreadPtr = IThread*;

class IThreadFactory : public IRef {
 public:
  virtual IThreadPtr Acquire(IFuncPtr f) = 0;

  virtual void Release(IThreadPtr t) = 0;
};

using IThreadFactoryPtr = IntrusivePtr<IThreadFactory>;

IThreadFactoryPtr MakeThreadFactory(std::size_t cache = 0);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, std::string_view name);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, std::size_t priority);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, IFuncPtr acquire, IFuncPtr release);

}  // namespace yaclib
