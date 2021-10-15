#pragma once

#include <yaclib/executor/task.hpp>
#include <yaclib/util/intrusive_node.hpp>

#include <cstddef>
#include <functional>
#include <memory>

namespace yaclib {

class IThread : public util::detail::Node {
 public:
  virtual ~IThread() = default;
};

using IThreadPtr = IThread*;

class IThreadFactory : public util::IRef {
 public:
  virtual IThreadPtr Acquire(util::IFuncPtr f) = 0;

  virtual void Release(IThreadPtr t) = 0;
};

using IThreadFactoryPtr = util::Ptr<IThreadFactory>;

IThreadFactoryPtr MakeThreadFactory(size_t cache = 0);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, std::string name);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, size_t priority);

IThreadFactoryPtr MakeThreadFactory(IThreadFactoryPtr base, util::IFuncPtr acquire, util::IFuncPtr release);

}  // namespace yaclib
