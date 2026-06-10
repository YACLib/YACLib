#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>

namespace yaclib {

template <typename V, typename T>
void Connect(FutureBase<V, T>&& f, Promise<V, T>&& p) {
  static_assert(std::is_move_constructible_v<wrap_void_t<V>>);
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

template <typename V, typename T>
void Connect(const SharedFutureBase<V, T>& f, Promise<V, T>&& p) {
  YACLIB_ASSERT(f.Valid());
  YACLIB_ASSERT(p.Valid());
  if (f.GetCore()->SetCallback(*p.GetCore().Get())) {
    p.GetCore().Release();
  } else {
    std::move(p).Set(f.Touch());
  }
}

template <typename V, typename T>
void Connect(FutureBase<V, T>&& f, SharedPromise<V, T>&& p) {
  YACLIB_ASSERT(f.Valid());
  YACLIB_ASSERT(p.Valid());
  if (f.GetCore()->SetCallback(*p.GetCore().Get())) {
    f.GetCore().Release();
    p.GetCore().Release();
  } else {
    std::move(p).Set(std::move(f).Touch());
  }
}

template <typename V, typename T>
void Connect(const SharedFutureBase<V, T>& f, SharedPromise<V, T>&& p) {
  YACLIB_ASSERT(f.Valid());
  YACLIB_ASSERT(p.Valid());
  YACLIB_ASSERT(f.GetCore() != p.GetCore());
  if (f.GetCore()->SetCallback(*p.GetCore().Get())) {
    p.GetCore().Release();
  } else {
    std::move(p).Set(f.Touch());
  }
}

template <typename V, typename T>
void Connect(SharedPromise<V, T>& primary, Promise<V, T>&& subsumed) {
  YACLIB_ASSERT(primary.Valid());
  YACLIB_ASSERT(subsumed.Valid());
  auto subsumed_core = subsumed.GetCore().Release();
  std::ignore = primary.GetCore()->SetCallback(*subsumed_core);
}

template <typename V, typename T>
void Connect(SharedPromise<V, T>& primary, SharedPromise<V, T>&& subsumed) {
  YACLIB_ASSERT(primary.Valid());
  YACLIB_ASSERT(subsumed.Valid());
  auto subsumed_core = subsumed.GetCore().Release();
  std::ignore = primary.GetCore()->SetCallback(*subsumed_core);
}

}  // namespace yaclib
