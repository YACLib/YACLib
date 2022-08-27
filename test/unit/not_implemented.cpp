#include <yaclib/async/contract.hpp>
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

void CheckHere() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.InlineCore::Here(core);
}

TEST(InlineCore, Here) {
#ifndef YACLIB_LOG_DEBUG
  CheckHere();
#else
  EXPECT_FATAL_FAILURE(CheckHere(), "");
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
