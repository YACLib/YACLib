#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/util/result.hpp>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(Result, VoidSizeof) {
  static_assert(sizeof(yaclib::Result<void>) == sizeof(std::exception_ptr) * 2);
  fprintf(stderr, "%lu\n", sizeof(yaclib::Result<void>));
}

TEST(Result, IntSizeof) {
  static_assert(sizeof(yaclib::Result<int>) == sizeof(std::exception_ptr) * 2);
  fprintf(stderr, "%lu\n", sizeof(yaclib::Result<int>));
}

TEST(Result, StringViewSizeof) {
  static_assert(sizeof(yaclib::Result<std::string_view>) == sizeof(void*) * 3);
  fprintf(stderr, "%lu\n", sizeof(yaclib::Result<std::string_view>));
}

TEST(Result, VectorSizeof) {
  static_assert(sizeof(yaclib::Result<std::vector<int>>) == sizeof(void*) * 4);
  fprintf(stderr, "%lu\n", sizeof(yaclib::Result<std::vector<int>>));
}

TEST(CCore, Sizeof) {
  using Core = yaclib::detail::CCore;
  static_assert(sizeof(Core) == sizeof(void*) * 5);
  fprintf(stderr, "%lu\n", sizeof(Core));
}

TEST(Core, EmptySizeof) {
  auto* core = yaclib::detail::MakeCore<yaclib::detail::CoreType::Run, void, yaclib::StopError>([] {
  });
  static_assert(sizeof(*core) == (sizeof(yaclib::detail::CCore) +  //
                                  sizeof(yaclib::Result<void>) +   //
                                  0));
  fprintf(stderr, "%lu\n", sizeof(*core));
  core->SetWait(yaclib::detail::PCore::kWaitStop);
  core->Drop();
}

void kek() {
}

TEST(Core, Sizeof) {
  auto* core = yaclib::detail::MakeCore<yaclib::detail::CoreType::Run, void, yaclib::StopError>(kek);
  static_assert(sizeof(*core) == (sizeof(yaclib::detail::CCore) +  //
                                  sizeof(yaclib::Result<void>) +   //
                                  sizeof(&kek) +                   //
                                  0));
  fprintf(stderr, "%lu\n", sizeof(*core));
  core->Drop();
  core->SetWait(yaclib::detail::PCore::kWaitStop);
}

}  // namespace
}  // namespace test
