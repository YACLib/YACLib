#pragma once
#include <yaclib/async/core.hpp>
#include <yaclib/async/future.hpp>

namespace yaclib::async {

template <typename T>
class Promise {
 public:
  Promise()
      : _state{container::intrusive::Ptr{
            new container::Counter<Core<T, void, void>>{}}} {
  }

  Future<T> MakeFuture() {
    assert(!_future_extracted);
    return Future<T>{_state};
  }

  void Set(T&& result) && {
    _state->SetResult(std::move(result));
  }

  void Set(const T& result) && {
    _state->SetResult(result);
  }

 private:
  bool _future_extracted{false};
  PromiseCorePtr<T> _state;
};

template <typename T>
using Contract = std::pair<Future<T>, Promise<T>>;

template <typename T>
Contract<T> MakeContract() {
  Promise<T> p;
  auto f{p.MakeFuture()};
  return {std::move(f), std::move(p)};
}

}  // namespace yaclib::async
