#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/util/result.hpp>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(Result, VoidSizeof) {
  static_assert(sizeof(yaclib::Result<void>) == sizeof(std::exception_ptr) + alignof(std::exception_ptr));
  fprintf(stderr, "%lu\n", sizeof(yaclib::Result<void>));
}

TEST(Result, IntSizeof) {
  static_assert(sizeof(yaclib::Result<int>) == sizeof(std::exception_ptr) + alignof(std::exception_ptr));
  fprintf(stderr, "%lu\n", sizeof(yaclib::Result<int>));
}

TEST(Result, StringViewSizeof) {
  static_assert(sizeof(yaclib::Result<std::string_view>) == sizeof(std::string_view) + alignof(std::exception_ptr));
  fprintf(stderr, "%lu\n", sizeof(yaclib::Result<std::string_view>));
}

TEST(Result, VectorSizeof) {
  static_assert(sizeof(yaclib::Result<std::vector<int>>) == sizeof(std::vector<int>) + alignof(std::exception_ptr));
  fprintf(stderr, "%lu\n", sizeof(yaclib::Result<std::vector<int>>));
}

TEST(BaseCore, Sizeof) {
  using Core = yaclib::detail::BaseCore;
#if YACLIB_FAULT != 2 || !defined(YACLIB_ATOMIC_EVENT)
  static_assert(sizeof(void*) == sizeof(int) || sizeof(Core) == sizeof(void*) * 5);
#endif
  fprintf(stderr, "%lu\n", sizeof(Core));
}

#if !defined(LAMBDA_SIZE) && defined(__has_cpp_attribute)
#  if __has_cpp_attribute(no_unique_address)
#    define LAMBDA_SIZE
constexpr size_t kZeroCaptureLambdaSizeof = 0;
#  endif
#endif

#if !defined(LAMBDA_SIZE) && defined(__has_attribute)
#  if __has_attribute(no_unique_address)
#    define LAMBDA_SIZE
constexpr size_t kZeroCaptureLambdaSizeof = 0;
#  endif
#endif

#ifndef LAMBDA_SIZE
constexpr size_t kZeroCaptureLambdaSizeof = sizeof(void*);
#endif

TEST(Core, EmptySizeof) {
  auto* core = yaclib::detail::MakeCore<yaclib::detail::CoreType::Run, void, yaclib::StopError>([] {
  });

  static_assert(sizeof(void*) == sizeof(int) || sizeof(*core) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                  sizeof(yaclib::Result<void>) +      //
                                                                  kZeroCaptureLambdaSizeof +          //
                                                                  0));
  fprintf(stderr, "%lu\n", sizeof(*core));
  core->SetWait(yaclib::detail::InlineCore::kWaitStop);
  core->Drop();
}

void kek() {
}

TEST(Core, Sizeof) {
  auto* core = yaclib::detail::MakeCore<yaclib::detail::CoreType::Run, void, yaclib::StopError>(kek);
  static_assert(sizeof(void*) == sizeof(int) || sizeof(*core) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                  sizeof(yaclib::Result<void>) +      //
                                                                  sizeof(&kek) +                      //
                                                                  0));
  fprintf(stderr, "%lu\n", sizeof(*core));
  core->Drop();
  core->SetWait(yaclib::detail::InlineCore::kWaitStop);
}

}  // namespace
}  // namespace test
