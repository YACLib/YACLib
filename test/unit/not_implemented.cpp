#include <yaclib/async/contract.hpp>

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
  EXPECT_FATAL_FAILURE(CallInlineState(), "");
}

void CallState() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.InlineCore::Call();
}

TEST(InlineCore, Call) {
  EXPECT_FATAL_FAILURE(CallState(), "");
}

void CancelState() {
  auto [f, p] = yaclib::MakeContract();
  auto& core = *p.GetCore();
  core.InlineCore::Cancel();
}

TEST(InlineCore, Cancel) {
  EXPECT_FATAL_FAILURE(CancelState(), "");
}

}  // namespace
}  // namespace test
