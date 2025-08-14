#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>

namespace yaclib {

template <typename V, typename E>
void Connect(FutureBase<V, E>&& f, Promise<V, E>&& p) {
  static_assert(std::is_move_constructible_v<Result<V, E>>);
  YACLIB_ASSERT(f.Valid());
  YACLIB_ASSERT(p.Valid());
  YACLIB_ASSERT(f.GetCore() != p.GetCore());
  if (f.GetCore()->SetCallback(*p.GetCore().Get())) {
    f.GetCore().Release();
    p.GetCore().Release();
  } else {
    std::move(p).Set(std::move(f).Touch());
  }
}

template <typename V, typename E>
void Connect(const SharedFuture<V, E>& f, Promise<V, E>&& p) {
  YACLIB_ASSERT(f.Valid());
  YACLIB_ASSERT(p.Valid());
  if (f.GetCore()->SetCallback(*p.GetCore().Get())) {
    p.GetCore().Release();
  } else {
    std::move(p).Set(f.Touch());
  }
}

template <typename V, typename E>
void Connect(FutureBase<V, E>&& f, SharedPromise<V, E>&& p) {
  YACLIB_ASSERT(f.Valid());
  YACLIB_ASSERT(p.Valid());
  if (f.GetCore()->SetCallback(*p.GetCore().Get())) {
    f.GetCore().Release();
    p.GetCore().Release();
  } else {
    std::move(p).Set(std::move(f).Touch());
  }
}

template <typename V, typename E>
void Connect(const SharedFuture<V, E>& f, SharedPromise<V, E>&& p) {
  YACLIB_ASSERT(f.Valid());
  YACLIB_ASSERT(p.Valid());
  YACLIB_ASSERT(f.GetCore() != p.GetCore());
  if (f.GetCore()->SetCallback(*p.GetCore().Get())) {
    p.GetCore().Release();
  } else {
    std::move(p).Set(f.Touch());
  }
}

template <typename V, typename E>
void Connect(SharedPromise<V, E>& primary, Promise<V, E>&& subsumed) {
  YACLIB_ASSERT(primary.Valid());
  YACLIB_ASSERT(subsumed.Valid());
  auto subsumed_core = subsumed.GetCore().Release();
  std::ignore = primary.GetCore()->SetCallback(*subsumed_core);
}

template <typename V, typename E>
void Connect(SharedPromise<V, E>& primary, SharedPromise<V, E>&& subsumed) {
  YACLIB_ASSERT(primary.Valid());
  YACLIB_ASSERT(subsumed.Valid());
  auto subsumed_core = subsumed.GetCore().Release();
  std::ignore = primary.GetCore()->SetCallback(*subsumed_core);
}

}  // namespace yaclib
