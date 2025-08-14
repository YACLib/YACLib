#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>

namespace yaclib {

// TODO(ocelaiwo) Make StoreCallback for shared cores and use here,
// memory orders can probably be relaxed and no need for std::ignore

template <typename V, typename E>
void Subsume(SharedPromise<V, E>& primary, Promise<V, E>&& subsumed) {
  YACLIB_ASSERT(primary.Valid());
  YACLIB_ASSERT(subsumed.Valid());
  auto subsumed_core = subsumed.GetCore().Release();
  std::ignore = primary.GetCore()->SetCallback(*subsumed_core);
}

template <typename V, typename E>
void Subsume(SharedPromise<V, E>& primary, SharedPromise<V, E>&& subsumed) {
  YACLIB_ASSERT(primary.Valid());
  YACLIB_ASSERT(subsumed.Valid());
  auto subsumed_core = subsumed.GetCore().Release();
  std::ignore = primary.GetCore()->SetCallback(*subsumed_core);
}

}  // namespace yaclib
