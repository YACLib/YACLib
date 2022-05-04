#include <yaclib/algo/when_any.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <array>
#include <chrono>
#include <exception>
#include <iosfwd>
#include <iterator>
#include <ratio>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <yaclib_std/chrono>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

enum class Result {
  Ok,
  Error,
  Exception,
};

enum class Container {
  Vector,
  Array,
  // TODO(Ri7ay) Tuple
};

enum class Value {
  Int,
  Void,
};

template <Container C, yaclib::WhenPolicy P, typename V, int kSize = 3, bool UseDefault = false>
auto FillArrays(std::array<yaclib::Promise<V>, kSize>& promises, std::array<yaclib::Future<V>, kSize>& futures) {
  for (int i = 0; i < kSize; ++i) {
    std::tie(futures[i], promises[i]) = yaclib::MakeContract<V>();
  }
  return [&futures] {
    if (UseDefault) {
      if constexpr (C == Container::Array) {
        return WhenAny(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
      } else {
        return WhenAny(futures.begin(), futures.end());
      }
    } else {
      if constexpr (C == Container::Array) {
        return yaclib::WhenAny<P>(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
      } else {
        return yaclib::WhenAny<P>(futures.begin(), futures.end());
      }
    }
  }();
}

template <Result R, Container C, yaclib::WhenPolicy P, typename V>
void JustWork() {
  constexpr int kSize = 3;
  std::array<yaclib::Promise<V>, kSize> promises;
  std::array<yaclib::Future<V>, kSize> futures;
  auto any = FillArrays<C, P, V, kSize>(promises, futures);

  EXPECT_TRUE(any.Valid());
  EXPECT_FALSE(any.Ready());

  if constexpr (std::is_void_v<V>) {
    std::move(promises[1]).Set();
  } else {
    std::move(promises[1]).Set(5);
  }

  EXPECT_TRUE(any.Ready());

  if constexpr (!std::is_void_v<V>) {
    std::move(promises[0]).Set(100);
    EXPECT_EQ(std::move(any).Get().Ok(), 5);
  }
}

template <Result R, Container C, yaclib::WhenPolicy P, typename V, bool UseDefault = false>
void Fail() {
  auto f1 = yaclib::StopError{yaclib::StopTag{}};
  auto f2 = std::make_exception_ptr(std::runtime_error{""});
  constexpr int kSize = 3;
  std::array<yaclib::Promise<V>, kSize> promises;
  std::array<yaclib::Future<V>, kSize> futures;
  auto any = FillArrays<C, P, V, kSize, UseDefault>(promises, futures);
  EXPECT_FALSE(any.Ready());
  if constexpr (R == Result::Error) {
    std::move(promises[1]).Set(f1);
  } else {
    std::move(promises[1]).Set(f2);
  }
  EXPECT_EQ(any.Ready(), P == yaclib::WhenPolicy::None);
  // Second and third error
  if constexpr (R == Result::Error) {
    std::move(promises[0]).Set(f2);
    std::move(promises[2]).Set(f2);
  } else {
    std::move(promises[0]).Set(f1);
    std::move(promises[2]).Set(f1);
  }
  EXPECT_TRUE(any.Ready());
  if constexpr ((P == yaclib::WhenPolicy::LastFail && R == Result::Exception) ||
                ((P == yaclib::WhenPolicy::None || P == yaclib::WhenPolicy::FirstFail) && R == Result::Error)) {
    EXPECT_THROW(std::move(any).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  } else {
    EXPECT_THROW(std::move(any).Get().Ok(), std::runtime_error);
  }
}

template <Result R, Container C, yaclib::WhenPolicy P, typename V>
void Default() {
  Fail<R, C, P, V, true>();
}

template <Result R, Container C, yaclib::WhenPolicy P, typename V>
void EmptyInput() {
  auto empty = std::vector<yaclib::Future<V>>{};
  auto any = yaclib::WhenAny<P>(empty.begin(), empty.end());
  EXPECT_FALSE(any.Valid());
  //  Return this code if we decide to return an empty ready future
  //  EXPECT_TRUE(any.Ready());
  //  auto result = std::move(any).Get();
  //  EXPECT_EQ(result.State(), ResultState::Empty);
  //  EXPECT_THROW(std::move(result).Ok(), ResultEmpty);
}

template <Result R, Container C, yaclib::WhenPolicy P, typename V>
void MultiThreaded() {
  static constexpr bool kIsVoid = std::is_void_v<V>;

  auto tp = yaclib::MakeThreadPool();

  auto async_value = [tp](int value) {
    return Run(*tp, [value] {
      yaclib_std::this_thread::sleep_for(20ms * YACLIB_CI_SLOWDOWN);
      if constexpr (kIsVoid) {
        std::ignore = value;
      } else {
        return value;
      }
    });
  };

  static const int kValues = 6;
  static const std::set<int> kOuts = {0, 1, 2, 3, 4, 5};

  std::array<yaclib::FutureOn<V>, 6> fs;
  for (int i = 0; i < kValues; ++i) {
    fs[static_cast<size_t>(i)] = async_value(i);
  }
  auto gen_fs = [&fs] {
    if constexpr (C == Container::Vector) {
      return yaclib::WhenAny<P>(fs.begin(), fs.end());
    } else {
      return yaclib::WhenAny<P>(std::move(fs[0]), std::move(fs[1]), std::move(fs[2]), std::move(fs[3]),
                                std::move(fs[4]), std::move(fs[5]));
    }
  };

  auto begin = yaclib_std::chrono::steady_clock::now();
  auto ints = gen_fs().Get();
  auto time = yaclib_std::chrono::steady_clock::now() - begin;
  EXPECT_LT(time, 40ms * YACLIB_CI_SLOWDOWN);

  if constexpr (kIsVoid) {
    EXPECT_NO_THROW(std::move(ints).Ok());
  } else {
    std::remove_reference_t<decltype(std::move(ints).Ok())> result;
    EXPECT_NO_THROW(result = std::move(ints).Ok());
    EXPECT_TRUE(kOuts.find(result) != kOuts.end());
  }

  tp->HardStop();
  tp->Wait();
}

template <Result R, Container C, yaclib::WhenPolicy P, typename V>
void TimeTest() {
  using Duration = std::chrono::nanoseconds;
  static constexpr bool is_void = std::is_void_v<V>;

  auto tp = yaclib::MakeThreadPool();

  auto async_value = [tp](int value, Duration d) {
    return Run(*tp, [value, d] {
      yaclib_std::this_thread::sleep_for(d * YACLIB_CI_SLOWDOWN);
      if constexpr (!is_void) {
        return value;
      } else {
        std::ignore = value;
      }
    });
  };

  static const int kValues = 2;

  std::array<yaclib::FutureOn<V>, kValues> fs;

  fs[0] = async_value(10, 100ms);
  fs[1] = async_value(5, 20ms);

  auto gen_fs = [&fs] {
    if constexpr (C == Container::Vector) {
      return yaclib::WhenAny<P>(fs.begin(), fs.end());
    } else {
      return yaclib::WhenAny<P>(std::move(fs[0]), std::move(fs[1]));
    }
  };

  auto begin = yaclib_std::chrono::steady_clock::now();
  auto ints = gen_fs().Get();
  auto time = yaclib_std::chrono::steady_clock::now() - begin;
  EXPECT_LT(time, 90ms * YACLIB_CI_SLOWDOWN);

  if constexpr (is_void) {
    EXPECT_NO_THROW(std::move(ints).Ok());
  } else {
    auto result = std::move(ints).Ok();
    EXPECT_EQ(result, 5);
  }

  tp->HardStop();
  tp->Wait();
}

std::string ToString(Result c) {
  switch (c) {
    case Result::Ok:
      return "Ok";
    case Result::Error:
      return "Error";
    case Result::Exception:
      return "Exception";
    default:
      return "Undefined";
  }
}

std::string ToString(Container c) {
  switch (c) {
    case Container::Vector:
      return "Vector";
    case Container::Array:
      return "Array";
    default:
      return "Undefined";
  }
}

std::string ToString(yaclib::WhenPolicy p) {
  switch (p) {
    case yaclib::WhenPolicy::None:
      return "None";
    case yaclib::WhenPolicy::FirstFail:
      return "FirstFail";
    case yaclib::WhenPolicy::LastFail:
      return "LastFail";
    default:
      return "Undefined";
  }
}
std::string ToString(Value v) {
  switch (v) {
    case Value::Int:
      return "Int";
    case Value::Void:
      return "Void";
    default:
      return "Undefined";
  }
}

using Param = std::tuple<Result, Container, yaclib::WhenPolicy, Value>;

std::string ToString(const Param& param) {
  return ToString(std::get<0>(param)) + "_" + ToString(std::get<1>(param)) + "_" + ToString(std::get<2>(param)) + "_" +
         ToString(std::get<3>(param));
}

class WhenAnyMatrix : public testing::TestWithParam<Param> {};

using WhenAnyOK = WhenAnyMatrix;
INSTANTIATE_TEST_SUITE_P(WhenAnyOK, WhenAnyOK,
                         testing::Combine(testing::Values(Result::Ok),
                                          testing::Values(Container::Vector, Container::Array),
                                          testing::Values(yaclib::WhenPolicy::None, yaclib::WhenPolicy::FirstFail,
                                                          yaclib::WhenPolicy::LastFail),
                                          testing::Values(Value::Int, Value::Void)),
                         [](const testing::TestParamInfo<WhenAnyMatrix::ParamType>& info) {
                           return ToString(info.param);
                         });

using WhenAnyFail = WhenAnyMatrix;
INSTANTIATE_TEST_SUITE_P(WhenAnyFail, WhenAnyFail,
                         testing::Combine(testing::Values(Result::Error, Result::Exception),
                                          testing::Values(Container::Vector, Container::Array),
                                          testing::Values(yaclib::WhenPolicy::None, yaclib::WhenPolicy::FirstFail,
                                                          yaclib::WhenPolicy::LastFail),
                                          testing::Values(Value::Int, Value::Void)),
                         [](const testing::TestParamInfo<WhenAnyMatrix::ParamType>& info) {
                           return ToString(info.param);
                         });

using WhenAnyEmpty = WhenAnyMatrix;
INSTANTIATE_TEST_SUITE_P(WhenAnyEmpty, WhenAnyEmpty,
                         testing::Combine(testing::Values(Result::Ok), testing::Values(Container::Vector),
                                          testing::Values(yaclib::WhenPolicy::None, yaclib::WhenPolicy::FirstFail,
                                                          yaclib::WhenPolicy::LastFail),
                                          testing::Values(Value::Int, Value::Void)),
                         [](const testing::TestParamInfo<WhenAnyMatrix::ParamType>& info) {
                           return ToString(info.param);
                         });

using WhenAnyDefault = WhenAnyMatrix;
INSTANTIATE_TEST_SUITE_P(WhenAnyDefault, WhenAnyDefault,
                         testing::Combine(testing::Values(Result::Error, Result::Exception),
                                          testing::Values(Container::Vector, Container::Array),
                                          testing::Values(yaclib::WhenPolicy::LastFail),
                                          testing::Values(Value::Int, Value::Void)),
                         [](const testing::TestParamInfo<WhenAnyMatrix::ParamType>& info) {
                           return ToString(info.param);
                         });

#define CALL_F_R_C_P(Function, Result, Container, Policy)                                                              \
  switch (v) {                                                                                                         \
    case Value::Int:                                                                                                   \
      return Function<Result, Container, Policy, int>();                                                               \
    case Value::Void:                                                                                                  \
      return Function<Result, Container, Policy, void>();                                                              \
    default:                                                                                                           \
      FAIL();                                                                                                          \
      return;                                                                                                          \
  }

#define CALL_F_R_C(Function, Result, Container)                                                                        \
  switch (p) {                                                                                                         \
    case yaclib::WhenPolicy::None:                                                                                     \
      CALL_F_R_C_P(Function, Result, Container, yaclib::WhenPolicy::None)                                              \
    case yaclib::WhenPolicy::FirstFail:                                                                                \
      CALL_F_R_C_P(Function, Result, Container, yaclib::WhenPolicy::FirstFail)                                         \
    case yaclib::WhenPolicy::LastFail:                                                                                 \
      CALL_F_R_C_P(Function, Result, Container, yaclib::WhenPolicy::LastFail)                                          \
    default:                                                                                                           \
      FAIL();                                                                                                          \
      return;                                                                                                          \
  }

#define CALL_F_R(Function, Result)                                                                                     \
  switch (c) {                                                                                                         \
    case Container::Vector:                                                                                            \
      CALL_F_R_C(Function, Result, Container::Vector)                                                                  \
    case Container::Array:                                                                                             \
      CALL_F_R_C(Function, Result, Container::Array)                                                                   \
    default:                                                                                                           \
      FAIL();                                                                                                          \
      return;                                                                                                          \
  }

#define CALL_F(Function)                                                                                               \
  switch (r) {                                                                                                         \
    case Result::Ok:                                                                                                   \
      CALL_F_R(Function, Result::Ok)                                                                                   \
    case Result::Error:                                                                                                \
      CALL_F_R(Function, Result::Error)                                                                                \
    case Result::Exception:                                                                                            \
      CALL_F_R(Function, Result::Exception)                                                                            \
    default:                                                                                                           \
      FAIL();                                                                                                          \
      return;                                                                                                          \
  }

TEST_P(WhenAnyOK, JustWork) {
  auto [r, c, p, v] = GetParam();
  EXPECT_EQ(r, Result::Ok);
  CALL_F(JustWork)
}

TEST_P(WhenAnyFail, Fail) {
  auto [r, c, p, v] = GetParam();
  EXPECT_NE(r, Result::Ok);
  CALL_F(Fail)
}

TEST_P(WhenAnyEmpty, EmptyInput) {
  auto [r, c, p, v] = GetParam();
  EXPECT_EQ(r, Result::Ok);
  EXPECT_EQ(c, Container::Vector);
  CALL_F(EmptyInput)
}

TEST_P(WhenAnyOK, MultiThreaded) {
  auto [r, c, p, v] = GetParam();
  EXPECT_EQ(r, Result::Ok);
  CALL_F(MultiThreaded)
}

TEST_P(WhenAnyOK, TimeTest) {
  auto [r, c, p, v] = GetParam();
  EXPECT_EQ(r, Result::Ok);
  CALL_F(TimeTest)
}

TEST_P(WhenAnyDefault, Default) {
  auto [r, c, p, v] = GetParam();
  EXPECT_NE(r, Result::Ok);
  EXPECT_EQ(p, yaclib::WhenPolicy::LastFail);
  CALL_F(Default)
}

TEST(WhenAny, LengthOne) {
  auto [f, p] = yaclib::MakeContract();
  const void* old_addr = p.GetCore().Get();
  auto new_future = WhenAny(&f, 1);
  const void* new_addr = new_future.GetCore().Get();
  EXPECT_EQ(old_addr, new_addr);
}

}  // namespace
}  // namespace test
