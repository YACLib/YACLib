#include <yaclib/async/connect.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/shared_contract.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>

#include <string_view>

#include <gtest/gtest.h>

namespace test {
namespace {

constexpr int kSetInt = 5;
constexpr std::string_view kSetString = "aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa";

template <typename V, typename E, bool Shared>
auto GetContract() {
  if constexpr (Shared) {
    return yaclib::MakeSharedContract<V, E>();
  } else {
    return yaclib::MakeContract<V, E>();
  }
}

template <typename L, typename R>
void doConnect(L& l, R& r) {
  if constexpr (yaclib::is_shared_future_base_v<L>) {
    Connect(l, std::move(r));
  } else {
    Connect(std::move(l), std::move(r));
  }
}

template <typename Future>
auto doGet(Future& f) {
  if constexpr (yaclib::is_shared_future_base_v<Future>) {
    return f.Get().Value();
  } else {
    return std::move(f).Get().Value();
  }
}

template <bool LShared, bool RShared, typename V = void, typename E = yaclib::StopError, typename Expected>
void FuturePromiseConnectSet(Expected expected) {
  auto [f1, p1] = GetContract<V, E, LShared>();
  auto [f2, p2] = GetContract<V, E, RShared>();
  doConnect(f1, p2);
  std::move(p1).Set(expected);
  ASSERT_EQ(doGet(f2), expected);
}

TEST(Connect, ConnectSet) {
  FuturePromiseConnectSet<false, false>(yaclib::Unit{});
  FuturePromiseConnectSet<false, true>(yaclib::Unit{});
  FuturePromiseConnectSet<true, false>(yaclib::Unit{});
  FuturePromiseConnectSet<true, true>(yaclib::Unit{});
  FuturePromiseConnectSet<false, false, int>(kSetInt);
  FuturePromiseConnectSet<false, true, int>(kSetInt);
  FuturePromiseConnectSet<true, false, int>(kSetInt);
  FuturePromiseConnectSet<true, true, int>(kSetInt);
  FuturePromiseConnectSet<false, false, std::string>(kSetString);
  FuturePromiseConnectSet<false, true, std::string>(kSetString);
  FuturePromiseConnectSet<true, false, std::string>(kSetString);
  FuturePromiseConnectSet<true, true, std::string>(kSetString);
}

template <bool LShared, bool RShared, typename V = void, typename E = yaclib::StopError, typename Expected>
void FuturePromiseSetConnect(Expected expected) {
  auto [f1, p1] = GetContract<V, E, LShared>();
  auto [f2, p2] = GetContract<V, E, RShared>();
  std::move(p1).Set(expected);
  doConnect(f1, p2);
  ASSERT_EQ(doGet(f2), expected);
}

TEST(Connect, SetConnect) {
  FuturePromiseSetConnect<false, false>(yaclib::Unit{});
  FuturePromiseSetConnect<false, true>(yaclib::Unit{});
  FuturePromiseSetConnect<true, false>(yaclib::Unit{});
  FuturePromiseSetConnect<true, true>(yaclib::Unit{});
  FuturePromiseSetConnect<false, false, int>(kSetInt);
  FuturePromiseSetConnect<false, true, int>(kSetInt);
  FuturePromiseSetConnect<true, false, int>(kSetInt);
  FuturePromiseSetConnect<true, true, int>(kSetInt);
  FuturePromiseSetConnect<false, false, std::string>(kSetString);
  FuturePromiseSetConnect<false, true, std::string>(kSetString);
  FuturePromiseSetConnect<true, false, std::string>(kSetString);
  FuturePromiseSetConnect<true, true, std::string>(kSetString);
}

TEST(Connect, SetSharedAfterContract) {
  auto [f, p] = yaclib::MakeContract<int>();
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  Connect(std::move(f), std::move(sp));
  sf = yaclib::SharedFuture<int>();
  std::move(p).Set(5);
}

TEST(Connect, ConnectLoops) {
  auto [f1, p1] = yaclib::MakeContract<int>();
  auto [f2, p2] = yaclib::MakeContract<int>();
  auto f3 = std::move(f2)
              .ThenInline([](int x) {
                return x + 1;
              })
              .ThenInline([](int x) {
                return x + 1;
              });
  std::move(p1).Set(kSetInt);
  Connect(std::move(f1), std::move(p2));
  ASSERT_EQ(std::move(f3).Get().Value(), kSetInt + 2);
}

struct MoveOnly {
  MoveOnly() = default;

  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;

  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly& operator=(MoveOnly&&) = default;
};

TEST(Connect, MoveOnly) {
  auto [f1, p1] = yaclib::MakeContract<MoveOnly>();
  auto [f2, p2] = yaclib::MakeContract<MoveOnly>();
  doConnect(f1, p2);
  std::move(p1).Set();
  std::ignore = std::move(f2).Get().Value();
}

TEST(Connect, ConnectToSharedPromise) {
  auto [pf, pp] = yaclib::MakeSharedContract<int>();
  auto [f, p] = yaclib::MakeContract<int>();
  auto [sf, sp] = yaclib::MakeSharedContract<int>();

  Connect(pp, std::move(p));
  Connect(pp, std::move(sp));

  std::move(pp).Set(kSetInt);

  ASSERT_EQ(pf.Get().Value(), kSetInt);
  ASSERT_EQ(std::move(f).Get().Value(), kSetInt);
  ASSERT_EQ(sf.Get().Value(), kSetInt);
}

}  // namespace
}  // namespace test
