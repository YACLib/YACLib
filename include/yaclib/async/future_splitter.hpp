#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/async/shared_promise.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/log.hpp>

namespace yaclib {
template <typename V, typename E>
class FutureSplitter {
 public:
  explicit FutureSplitter(Future<V, E>&& future) : _shared_promise{} {
    std::move(future)
      .Then(yaclib::MakeInline(),
            [&](V&& result) {  // TODO Result<V,E>
              _shared_promise.Set(std::move(result));
            })
      .Detach();
  }

  Future<V, E> GetFuture() {
    auto&& res = _shared_promise.GetFuture();
    if (res.Ready()) {
      std::cout << "In splitter bad!" << std::endl;
    }
    return res;
  }

 private:
  SharedPromise<V, E> _shared_promise;
};
}  // namespace yaclib
