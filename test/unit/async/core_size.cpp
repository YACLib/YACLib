#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/util/result.hpp>

#include <algorithm>
#include <iostream>

#include <gtest/gtest.h>

namespace test {
namespace {

// Mirrors the Result storage idiom, so the expectation below holds on every compiler,
// whether or not YACLIB_NO_UNIQUE_ADDRESS collapses the empty value
union ProbeState {
  YACLIB_RESULT_EMPTY_VALUE yaclib::Unit stub;

  ProbeState() noexcept : stub{} {
  }
  ~ProbeState() noexcept {
  }
};

struct Probe {
  std::exception_ptr error;
  YACLIB_RESULT_EMPTY_VALUE ProbeState state;
};

// Mirrors Core's base layout (ResultCore with the result union + FuncCore with the functor
// storage union), so the empty functor contribution matches what the real compiler produces
struct ProbeResultCore {
  void* stub[4];
  union {
    char result[16];
  };
};

struct ProbeFuncCore {
  YACLIB_NO_UNIQUE_ADDRESS ProbeState func;
};

struct ProbeCore : ProbeResultCore, ProbeFuncCore {};

constexpr std::size_t kZeroCaptureLambdaSizeof = sizeof(ProbeCore) - sizeof(ProbeResultCore);

constexpr std::size_t ExpectedResultSizeof(std::size_t size, std::size_t align) {
  const auto a = align > alignof(std::exception_ptr) ? align : alignof(std::exception_ptr);
  return (sizeof(std::exception_ptr) + size + a - 1) / a * a;
}

TEST(Result, VoidSizeof) {
  // Empty value is stored via YACLIB_NO_UNIQUE_ADDRESS, so where the attribute is supported
  // Result<void> is exactly one std::exception_ptr
  static_assert(sizeof(yaclib::Result<>) == sizeof(Probe));
  std::cout << "sizeof(yaclib::Result<>): " << sizeof(yaclib::Result<>) << std::endl;
}

TEST(Result, IntSizeof) {
  static_assert(sizeof(yaclib::Result<int>) == ExpectedResultSizeof(sizeof(int), alignof(int)));
  std::cout << "sizeof(yaclib::Result<int>): " << sizeof(yaclib::Result<int>) << std::endl;
}

TEST(Result, StringViewSizeof) {
  static_assert(sizeof(yaclib::Result<std::string_view>) ==
                ExpectedResultSizeof(sizeof(std::string_view), alignof(std::string_view)));
  std::cout << "sizeof(yaclib::Result<std::string_view>): " << sizeof(yaclib::Result<std::string_view>) << std::endl;
}

TEST(Result, VectorSizeof) {
  static_assert(sizeof(yaclib::Result<std::vector<int>>) ==
                ExpectedResultSizeof(sizeof(std::vector<int>), alignof(std::vector<int>)));
  std::cout << "sizeof(yaclib::Result<std::vector<int>>): " << sizeof(yaclib::Result<std::vector<int>>) << std::endl;
}

TEST(BaseCore, Sizeof) {
  using Core = yaclib::detail::BaseCore;
#if YACLIB_FAULT != 2 || YACLIB_FUTEX == 0
  static_assert(sizeof(void*) == sizeof(int) || sizeof(Core) == sizeof(void*) * 4);
#endif
  std::cout << "sizeof(yaclib::detail::BaseCore): " << sizeof(Core) << std::endl;
}

void kek() {
}

TEST(Core, EmptySizeof) {
  using CoreType = yaclib::detail::CoreType;

  static constexpr auto UniqueCoreT = CoreType::Run | CoreType::ToUnique | CoreType::Call;
  auto* unique = yaclib::detail::MakeCore<UniqueCoreT, void, yaclib::DefaultTrait>([] {
    kek();
  });
  constexpr auto kResultUnion = std::max(sizeof(yaclib::Result<>), sizeof(yaclib::detail::Callback));
  static_assert(sizeof(void*) == sizeof(int) || sizeof(*unique) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                    kResultUnion +                      //
                                                                    kZeroCaptureLambdaSizeof +          //
                                                                    0));
  std::cout << "sizeof(yaclib::MakeCore, ToUnique, zero capture lambda): " << sizeof(*unique) << std::endl;

  static constexpr auto SharedCoreT = CoreType::Run | CoreType::ToShared | CoreType::Call;
  auto* shared = yaclib::detail::MakeCore<SharedCoreT, void, yaclib::DefaultTrait>([] {
    kek();
  });
  static_assert(sizeof(void*) == sizeof(int) || sizeof(*shared) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                    kResultUnion +                      //
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
  auto* unique = yaclib::detail::MakeCore<UniqueCoreT, void, yaclib::DefaultTrait>(kek);
  constexpr auto kResultUnion = std::max(sizeof(yaclib::Result<>), sizeof(yaclib::detail::Callback));
  static_assert(sizeof(void*) == sizeof(int) || sizeof(*unique) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                    kResultUnion +                      //
                                                                    sizeof(&kek) +                      //
                                                                    0));
  std::cout << "sizeof(yaclib::MakeCore, ToUnique, function): " << sizeof(*unique) << std::endl;

  static constexpr auto SharedCoreT = CoreType::Run | CoreType::ToShared | CoreType::Call;
  auto* shared = yaclib::detail::MakeCore<SharedCoreT, void, yaclib::DefaultTrait>(kek);
  static_assert(sizeof(void*) == sizeof(int) || sizeof(*shared) == (sizeof(yaclib::detail::BaseCore) +  //
                                                                    kResultUnion +                      //
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
