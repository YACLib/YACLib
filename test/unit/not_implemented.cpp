#include <yaclib/async/contract.hpp>
#include <yaclib/config.hpp>
#include <yaclib/executor/detail/unique_job.hpp>

#if YACLIB_CORO
#  include <yaclib/coroutine/await_group.hpp>
#  include <yaclib/coroutine/detail/promise_type.hpp>
#  include <yaclib/executor/inline.hpp>
#  include <yaclib/util/detail/nope_counter.hpp>
#endif

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

namespace test {
namespace {

void CallInlineState() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.InlineCore::Here(core, yaclib::detail::InlineCore::State::kEmpty);
}

TEST(InlineCore, CallInline) {
#ifndef YACLIB_LOG_ERROR
  CallInlineState();
#else
  EXPECT_FATAL_FAILURE(CallInlineState(), "");
#endif
}

void CallState() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.InlineCore::Call();
}

TEST(InlineCore, Call) {
#ifndef YACLIB_LOG_ERROR
  CallState();
#else
  EXPECT_FATAL_FAILURE(CallState(), "");
#endif
}

void DropState() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.InlineCore::Drop();
}

TEST(InlineCore, Drop) {
#ifndef YACLIB_LOG_ERROR
  DropState();
#else
  EXPECT_FATAL_FAILURE(DropState(), "");
#endif
}

TEST(UniqueJob, Ref) {
  auto task = yaclib::detail::MakeUniqueJob([] {
  });
  task->IncRef();
  task->DecRef();
  task->Drop();
}

TEST(CoroDummy, DestroyResume) {
#if YACLIB_CORO
  yaclib::detail::Destroy d;
  d.await_resume();
#else
  GTEST_SKIP();
#endif
}

TEST(CoroDummy, BaseCoroGetHandle) {
#if YACLIB_CORO
  yaclib::detail::UniqueCounter<yaclib::detail::ResultCore<void, yaclib::StopError>> core;
  core.IncRef();
  core.Store(yaclib::Unit{});
  std::ignore = core.GetHandle();
#else
  GTEST_SKIP();
#endif
}

TEST(AwaitGroupDummy, Cancel) {
#if YACLIB_CORO
  yaclib::OneShotEventWait tmp;
  tmp.Drop();

  yaclib::detail::NopeCounter<yaclib::OneShotEvent> event;

  yaclib::OneShotEventAwaiter tmp2{yaclib::MakeInline(), event};
  // tmp2.Drop();
#else
  GTEST_SKIP();
#endif
}

}  // namespace
}  // namespace test
