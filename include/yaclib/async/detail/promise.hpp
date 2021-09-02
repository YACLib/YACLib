#pragma once
#include <yaclib/async/detail/core.hpp>

#include <utility>

namespace yaclib::async {

template <typename T>
class Future;

template <typename T>
class Promise {
  static_assert(!util::IsResultV<T>, "Promise cannot be instantiated with Result");
  static_assert(!util::IsFutureV<T>, "Promise cannot be instantiated with Future");
  static_assert(!std::is_same_v<util::DecayT<T>, std::error_code>,
                "Promise cannot be instantiated with std::error_code");
  static_assert(!std::is_same_v<util::DecayT<T>, std::exception_ptr>,
                "Promise cannot be instantiated with std::exception_ptr");

 public:
  Promise();
  Promise(Promise&& other) noexcept = default;
  Promise& operator=(Promise&& other) noexcept = default;
  Promise(const Promise& other) = delete;
  Promise& operator=(const Promise& other) = delete;

  Future<T> MakeFuture();

  template <typename Type>
  void Set(Type&& value) &&;

  void Set() &&;

 private:
  bool _future_extracted{false};
  detail::PromiseCorePtr<T> _core;
};

template <typename T>
struct Contract {
  Future<T> future;
  Promise<T> promise;
};

template <typename T>
Contract<T> MakeContract();

}  // namespace yaclib::async
