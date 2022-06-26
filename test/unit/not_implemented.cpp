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
  core.InlineCore::CallInline(core, yaclib::detail::InlineCore::State::Empty);
}

TEST(InlineCore, CallInline) {
#ifndef YACLIB_LOG_ERROR
  GTEST_SKIP();
#endif
  EXPECT_FATAL_FAILURE(CallInlineState(), "");
}

void CallState() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.InlineCore::Call();
}

TEST(InlineCore, Call) {
#ifndef YACLIB_LOG_ERROR
  GTEST_SKIP();
#endif
  EXPECT_FATAL_FAILURE(CallState(), "");
}

void CancelState() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.InlineCore::Cancel();
}

TEST(InlineCore, Cancel) {
#ifndef YACLIB_LOG_ERROR
  GTEST_SKIP();
#endif
  EXPECT_FATAL_FAILURE(CancelState(), "");
}

TEST(UniqueJob, IncRef) {
  auto task = yaclib::detail::MakeUniqueJob([] {
  });
  task->IncRef();
  task->DecRef();
}

TEST(CoroDummy, DestroyResume) {
#if YACLIB_CORO
  yaclib::detail::Destroy<void, yaclib::StopError> d;
  d.await_resume();
#else
  GTEST_SKIP();
#endif
}

TEST(CoroDummy, BaseCoroGetHandle) {
#if YACLIB_CORO
  yaclib::detail::NopeCounter<yaclib::detail::BaseCore> core{yaclib::detail::InlineCore::State::Empty};
  std::ignore = core.GetHandle();
#else
  GTEST_SKIP();
#endif
}

TEST(AwaitGroupDummy, Cancel) {
#if YACLIB_CORO
  yaclib::detail::NopeCounter<yaclib::OneShotEventWait> tmp;
  tmp.Cancel();

  yaclib::detail::NopeCounter<yaclib::OneShotEvent> event;

  yaclib::detail::NopeCounter<yaclib::OneShotEventAwaiter> tmp2(yaclib::MakeInline(), event);
  tmp2.Cancel();
#else
  GTEST_SKIP();
#endif
}

}  // namespace
}  // namespace test
