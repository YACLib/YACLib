#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/exe/thread_factory.hpp>
#include <yaclib/exe/thread_pool.hpp>

#include <cstddef>
#include <utility>
#include <yaclib_std/thread_local>

namespace yaclib {

static YACLIB_THREAD_LOCAL_PTR(IExecutor) tlCurrentThreadPool{&MakeInline()};

IExecutor& CurrentThreadPool() noexcept {
  return *tlCurrentThreadPool;
}

void SetCurrentThreadPool(IExecutor& executor) noexcept {
  tlCurrentThreadPool = &executor;
}

IThreadPoolPtr MakeSingleThread(IThreadFactoryPtr tf) {
  return {};
}

IThreadPoolPtr MakeFairThreadPool(std::size_t threads, IThreadFactoryPtr tf);

IThreadPoolPtr MakeGolangThreadPool(std::size_t threads);

// TODO(kononovk): choose thread pool type depending on threads and tf values, add function description
IThreadPoolPtr MakeThreadPool(std::size_t threads, IThreadFactoryPtr tf) {
  //  if (threads == 1) {
  //    return MakeSingleThread(std::move(tf));
  //  }
  //  return MakeGolangThreadPool(threads, std::move(tf));
  return MakeFairThreadPool(threads, std::move(tf));
}

// TODO(kononovk): validate thread pool type, threads and tf values, add function description
IThreadPoolPtr MakeThreadPool(std::size_t threads, IThreadFactoryPtr tf, IExecutor::Type type) {
  switch (type) {
    case IExecutor::Type::SingleThread:
      if (threads == 1) {
        return MakeSingleThread(std::move(tf));
      }
      YACLIB_WARN(true, "Impossible to create SingleThread with threads != 1");
      break;
    case IExecutor::Type::FairThreadPool:
      return MakeFairThreadPool(threads, std::move(tf));
    case IExecutor::Type::GolangThreadPool:
      return MakeGolangThreadPool(threads);
    case IExecutor::Type::Custom:
      [[fallthrough]];
    case IExecutor::Type::Inline:
      [[fallthrough]];
    case IExecutor::Type::Manual:
      [[fallthrough]];
    case IExecutor::Type::Strand:
      [[fallthrough]];
    default:
      YACLIB_WARN(true, "Invalid IExecutor::Type");
      break;
  }
  return nullptr;
}

}  // namespace yaclib
