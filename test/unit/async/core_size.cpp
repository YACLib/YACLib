#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/util/result.hpp>

#include <iostream>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(Result, VoidSizeof) {
  static_assert(sizeof(yaclib::Result<>) == sizeof(std::exception_ptr) + alignof(std::exception_ptr));
  std::cout << "sizeof(yaclib::Result<>): " << sizeof(yaclib::Result<>) << std::endl;
}

TEST(Result, IntSizeof) {
  static_assert(sizeof(yaclib::Result<int>) == sizeof(std::exception_ptr) + alignof(std::exception_ptr));
  std::cout << "sizeof(yaclib::Result<int>): " << sizeof(yaclib::Result<int>) << std::endl;
}

TEST(Result, StringViewSizeof) {
  static_assert(sizeof(yaclib::Result<std::string_view>) == sizeof(std::string_view) + alignof(std::exception_ptr));
  std::cout << "sizeof(yaclib::Result<std::string_view>): " << sizeof(yaclib::Result<std::string_view>) << std::endl;
}

TEST(Result, VectorSizeof) {
  static_assert(sizeof(yaclib::Result<std::vector<int>>) == sizeof(std::vector<int>) + alignof(std::exception_ptr));
  std::cout << "sizeof(yaclib::Result<std::vector<int>>): " << sizeof(yaclib::Result<std::vector<int>>) << std::endl;
}

TEST(BaseCore, Sizeof) {
  using Core = yaclib::detail::BaseCore;
#if YACLIB_FAULT != 2 || YACLIB_FUTEX == 0
  static_assert(sizeof(void*) == sizeof(int) || sizeof(Core) == sizeof(void*) * 4);
#endif
  std::cout << "sizeof(yaclib::detail::BaseCore): " << sizeof(Core) << std::endl;
}

#if !defined(LAMBDA_SIZE) && defined(__has_cpp_attribute)
#  if __has_cpp_attribute(no_unique_address)
#    define LAMBDA_SIZE
constexpr std::size_t kZeroCaptureLambdaSizeof = 0;
#  endif
#endif

#if !defined(LAMBDA_SIZE) && defined(__has_attribute)
#  if __has_attribute(no_unique_address)
#    define LAMBDA_SIZE
constexpr std::size_t kZeroCaptureLambdaSizeof = 0;
#  endif
#endif

#ifndef LAMBDA_SIZE
constexpr std::size_t kZeroCaptureLambdaSizeof = sizeof(void*);
#endif

void kek() {
}

TEST(Core, EmptySizeof) {
  using CoreType = yaclib::detail::CoreType;

  static constexpr auto UniqueCoreT = CoreType::Run | CoreType::ToUnique | CoreType::Call;
  auto* unique = yaclib::detail::MakeCore<UniqueCoreT, void, yaclib::StopError>([] {
    kek();
  });
  static_assert(sizeof(void*) == sizeof(int) || sizeof(*unique) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                    sizeof(yaclib::Result<>) +          //
                                                                    kZeroCaptureLambdaSizeof +          //
                                                                    0));
  std::cout << "sizeof(yaclib::MakeCore, ToUnique, zero capture lambda): " << sizeof(*unique) << std::endl;

  static constexpr auto SharedCoreT = CoreType::Run | CoreType::ToShared | CoreType::Call;
  auto* shared = yaclib::detail::MakeCore<SharedCoreT, void, yaclib::StopError>([] {
    kek();
  });
  static_assert(sizeof(void*) == sizeof(int) || sizeof(*shared) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                    sizeof(yaclib::Result<>) +          //
                                                                    kZeroCaptureLambdaSizeof +          //
                                                                    sizeof(std::size_t)));
  std::cout << "sizeof(yaclib::MakeCore, ToShared, zero capture lambda): " << sizeof(*shared) << std::endl;

  unique->StoreCallback(yaclib::detail::MakeDrop());
  static_cast<yaclib::Job*>(unique)->Drop();

  std::ignore = shared->SetCallback(yaclib::detail::MakeDrop());
  static_cast<yaclib::Job*>(shared)->Drop();
}

TEST(Core, Sizeof) {
  using CoreType = yaclib::detail::CoreType;

  static constexpr auto UniqueCoreT = CoreType::Run | CoreType::ToUnique | CoreType::Call;
  auto* unique = yaclib::detail::MakeCore<UniqueCoreT, void, yaclib::StopError>(kek);
  static_assert(sizeof(void*) == sizeof(int) || sizeof(*unique) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                    sizeof(yaclib::Result<>) +          //
                                                                    sizeof(&kek) +                      //
                                                                    0));
  std::cout << "sizeof(yaclib::MakeCore, ToUnique, function): " << sizeof(*unique) << std::endl;

  static constexpr auto SharedCoreT = CoreType::Run | CoreType::ToShared | CoreType::Call;
  auto* shared = yaclib::detail::MakeCore<SharedCoreT, void, yaclib::StopError>(kek);
  static_assert(sizeof(void*) == sizeof(int) || sizeof(*shared) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                    sizeof(yaclib::Result<>) +          //
                                                                    sizeof(&kek) +                      //
                                                                    sizeof(std::size_t)));
  std::cout << "sizeof(yaclib::MakeCore, ToShared, zero capture lambda): " << sizeof(*shared) << std::endl;

  unique->StoreCallback(yaclib::detail::MakeDrop());
  static_cast<yaclib::Job*>(unique)->Drop();

  std::ignore = shared->SetCallback(yaclib::detail::MakeDrop());
  static_cast<yaclib::Job*>(shared)->Drop();
}

}  // namespace
}  // namespace test
