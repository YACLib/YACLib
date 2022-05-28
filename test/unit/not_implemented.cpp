#include <yaclib/async/contract.hpp>
#include <yaclib/config.hpp>
#include <yaclib/exe/detail/unique_job.hpp>

#if YACLIB_CORO
#  include <yaclib/coro/detail/promise_type.hpp>
#endif

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

namespace test {
namespace {

void CallInlineState() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.PCore::Here(core, yaclib::detail::PCore::State::kEmpty);
}

TEST(PCore, CallInline) {
#ifndef YACLIB_LOG_ERROR
  GTEST_SKIP();
#endif
  EXPECT_FATAL_FAILURE(CallInlineState(), "");
}

void CallState() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.PCore::Call();
}

TEST(PCore, Call) {
#ifndef YACLIB_LOG_ERROR
  GTEST_SKIP();
#endif
  EXPECT_FATAL_FAILURE(CallState(), "");
}

void DropState() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.PCore::Drop();
}

TEST(PCore, Drop) {
#ifndef YACLIB_LOG_ERROR
  GTEST_SKIP();
#endif
  EXPECT_FATAL_FAILURE(DropState(), "");
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
  yaclib::detail::Destroy<void, yaclib::StopError> d;
  d.await_resume();
#else
  GTEST_SKIP();
#endif
}

}  // namespace
}  // namespace test
