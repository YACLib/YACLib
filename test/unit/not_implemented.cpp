#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/config.hpp>
#include <yaclib/exe/detail/unique_job.hpp>
#include <yaclib/util/detail/default_event.hpp>
#include <yaclib/util/detail/mutex_event.hpp>

#if YACLIB_CORO != 0
#  include <yaclib/coro/detail/promise_type.hpp>
#  include <yaclib/exe/inline.hpp>
#endif

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

namespace test {
namespace {

void CheckCall() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.Job::Call();
}

TEST(InlineCore, Call) {
#ifndef YACLIB_LOG_DEBUG
  CheckCall();
#else
  EXPECT_FATAL_FAILURE(CheckCall(), "");
#endif
}

void CheckDrop() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.Job::Drop();
}

TEST(InlineCore, Drop) {
#ifndef YACLIB_LOG_DEBUG
  CheckDrop();
#else
  EXPECT_FATAL_FAILURE(CheckDrop(), "");
#endif
}

#if YACLIB_CORO != 0
void CheckCurr() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  std::ignore = core.BaseCore::Curr();
}

TEST(BaseCore, Curr) {
#  ifndef YACLIB_LOG_DEBUG
  CheckCurr();
#  else
  EXPECT_FATAL_FAILURE(CheckCurr(), "");
#  endif
}
#endif

struct NonMovable {
  NonMovable() = default;
  NonMovable(const NonMovable&) = delete;
  NonMovable(NonMovable&&) = delete;
};

void CheckResultCoreImplNonMovable() {
  auto [f1, p1] = yaclib::MakeContract<NonMovable>();
  auto [f2, p2] = yaclib::MakeContract<NonMovable>();
  auto* f1_core = f1.GetCore().Release();
  auto* p2_core = p2.GetCore().Get();
  std::ignore = f1_core->TryAddCallback(*p2_core);
  std::move(p1).Set();
  f1_core->DecRef();
}

TEST(ResultCore, ImplNonMovable) {
#ifndef YACLIB_LOG_DEBUG
  CheckResultCoreImplNonMovable();
#else
  EXPECT_FATAL_FAILURE(CheckResultCoreImplNonMovable(), "");
#endif
}

TEST(UniqueJob, Ref) {
  auto task = yaclib::detail::MakeUniqueJob([] {
  });
  task->IncRef();
  task->DecRef();
  task->Drop();
}

TEST(MutexEvent, Reset) {
  yaclib::detail::MutexEvent m;
  m.Reset();
}

TEST(DefaultEvent, Reset) {
  yaclib::detail::DefaultEvent m;
  m.Reset();
}

TEST(CoroDummy, DestroyResume) {
#if YACLIB_CORO != 0
  yaclib::detail::Destroy d;
  d.await_resume();
#else
  GTEST_SKIP();
#endif
}

}  // namespace
}  // namespace test
