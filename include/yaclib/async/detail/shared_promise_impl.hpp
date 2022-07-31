#pragma once

#include <yaclib_std/detail/atomic.hpp>

#include <yaclib/async/future.hpp>

#include <iostream>  // fpr debug, remove later
#include <tuple>     // tie

namespace yaclib {
template <typename V, typename E>
template <typename Type>
void SharedPromise<V, E>::Set(Type&& value) {
  std::cout << "Set Called" << std::endl;
  static_assert(std::is_constructible_v<Result<V, E>, Type>, "TODO(MBkkt): Add message");
  _result = std::forward<Type>(value);
  ResultCoreType* head = _head.exchange(nullptr);
  _is_set.store(true, std::memory_order_seq_cst);  // TODO memory order
  while (head) {
    head->Set(_result.Value());
    head = static_cast<ResultCoreType*>(head->next);
  }
}

template <typename V, typename E>
void SharedPromise<V, E>::Set() {
  Set(Unit{});
}

template <typename V, typename E>
bool SharedPromise<V, E>::Valid() const& noexcept {
  // return _head.load() != nullptr;
  return false;  // Useless
}

template <typename V, typename E>
SharedPromise<V, E>::~SharedPromise() {
  ResultCoreType* head = _head.load();
  while (head) {
    head->Set(StopTag{});
    head = static_cast<ResultCoreType*>(head->next);
  }
}

template <typename V, typename E>
Future<V, E> SharedPromise<V, E>::GetFuture() {
  Future<V, E> future;
  Promise<V, E> promise;
  std::tie(future, promise) = yaclib::MakeContract<V, E>();
  ResultCoreType* old_head = _head.load();
  promise.GetCore()->next = old_head;
  while (_is_set.load() == false && !_head.compare_exchange_weak(old_head, &*promise.GetCore())) {
    promise.GetCore()->next = old_head;
  }

  if (_is_set.load()) {
    std::cout << "_is_set load" << std::endl;
    _head.store(nullptr);
    promise.GetCore()->next = nullptr;
    promise.GetCore()->Set(_result.Value());
  }
  if (future.Ready()) {
    std::cout << "SHIT!" << std::endl;
  } else {
    std::cout << "NOT READY!!" << std::endl;
  }
  return future;
}

}  // namespace yaclib
