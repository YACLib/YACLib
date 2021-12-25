#include <util/intrusive_list.hpp>

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <cassert>
#include <utility>

namespace yaclib {
namespace {

thread_local IThreadPool* tlCurrentThreadPool;

}  // namespace

IThreadPool* CurrentThreadPool() noexcept {
  return tlCurrentThreadPool;
}

void SetCurrentThreadPool(IThreadPool* tp) noexcept {
  tlCurrentThreadPool = tp;
}

IThreadPoolPtr MakeCommonThreadPool(size_t threads, IThreadFactoryPtr tf);

IThreadPoolPtr MakeThreadPool(size_t threads, IThreadFactoryPtr tf) {
  return MakeCommonThreadPool(threads, std::move(tf));
}

IThreadPoolPtr MakeThreadPool(IExecutor::Type tag, size_t threads, IThreadFactoryPtr tf) {
  if (tag == IExecutor::Type::ThreadPool) {
    return MakeCommonThreadPool(threads, std::move(tf));
  }

  assert(false);
  return nullptr;
}

}  // namespace yaclib
