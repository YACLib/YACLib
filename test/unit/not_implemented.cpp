#include <yaclib/async/contract.hpp>
#include <yaclib/config.hpp>
#include <yaclib/executor/detail/unique_job.hpp>

#if YACLIB_CORO
#  include <yaclib/coroutine/detail/promise_type.hpp>
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
  yaclib::detail::Destroy<void, yaclib::StopError>::await_resume();
#else
  GTEST_SKIP();
#endif
}

}  // namespace
}  // namespace test
