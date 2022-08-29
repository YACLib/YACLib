#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/util/result.hpp>

#include <iostream>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(Result, VoidSizeof) {
  static_assert(sizeof(yaclib::Result<>) == sizeof(std::exception_ptr) + alignof(std::exception_ptr));
  std::cerr << sizeof(yaclib::Result<>) << std::endl;
}

TEST(Result, IntSizeof) {
  static_assert(sizeof(yaclib::Result<int>) == sizeof(std::exception_ptr) + alignof(std::exception_ptr));
  std::cerr << sizeof(yaclib::Result<int>) << std::endl;
}

TEST(Result, StringViewSizeof) {
  static_assert(sizeof(yaclib::Result<std::string_view>) == sizeof(std::string_view) + alignof(std::exception_ptr));
  std::cerr << sizeof(yaclib::Result<std::string_view>) << std::endl;
}

TEST(Result, VectorSizeof) {
  static_assert(sizeof(yaclib::Result<std::vector<int>>) == sizeof(std::vector<int>) + alignof(std::exception_ptr));
  std::cerr << sizeof(yaclib::Result<std::vector<int>>) << std::endl;
}

TEST(BaseCore, Sizeof) {
  using Core = yaclib::detail::BaseCore;
#if YACLIB_FAULT != 2 || YACLIB_FUTEX == 0
  static_assert(sizeof(void*) == sizeof(int) || sizeof(Core) == sizeof(void*) * 5);
#endif
  std::cerr << sizeof(Core) << std::endl;
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

void kek() {
}

TEST(Core, EmptySizeof) {
  auto* core = yaclib::detail::MakeCore<yaclib::detail::CoreType::Run, void, yaclib::StopError>([] {
    kek();
  });

  static_assert(sizeof(void*) == sizeof(int) || sizeof(*core) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                  sizeof(yaclib::Result<>) +          //
                                                                  kZeroCaptureLambdaSizeof +          //
                                                                  0));
  std::cerr << sizeof(*core) << std::endl;

  core->StoreCallback(yaclib::detail::MakeEmpty(), yaclib::detail::BaseCore::kWaitDrop);
  core->Drop();
}

TEST(Core, Sizeof) {
  auto* core = yaclib::detail::MakeCore<yaclib::detail::CoreType::Run, void, yaclib::StopError>(kek);
  static_assert(sizeof(void*) == sizeof(int) || sizeof(*core) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                  sizeof(yaclib::Result<>) +          //
                                                                  sizeof(&kek) +                      //
                                                                  0));
  std::cerr << sizeof(*core) << std::endl;

  core->StoreCallback(yaclib::detail::MakeEmpty(), yaclib::detail::BaseCore::kWaitDrop);
  core->Drop();
}

}  // namespace
}  // namespace test
