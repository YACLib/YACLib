#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <utility>

namespace yaclib::detail {

template <typename V, typename E>
class ResultCore : public CCore {
 public:
  ResultCore() noexcept : CCore{kEmpty} {
  }

  template <typename... Args>
  explicit ResultCore(Args&&... args) noexcept(std::is_nothrow_constructible_v<Result<V, E>, Args...>)
      : CCore{kResult}, _result{std::forward<Args>(args)...} {
  }

  ~ResultCore() noexcept override {
    _result.~Result<V, E>();
  }

  template <typename T>
  void Store(T&& object) noexcept(std::is_nothrow_constructible_v<Result<V, E>, T>) {
    new (&_result) Result<V, E>{std::forward<T>(object)};
  }

  [[nodiscard]] Result<V, E>& Get() noexcept {
    return _result;
  }

  template <typename T, typename Func>
  void Done(T&& object, Func&& f) noexcept(std::is_nothrow_constructible_v<Result<V, E>, T>) {
    Store(std::forward<T>(object));
    std::forward<Func>(f)();
    SetResult();
  }

  void Drop() noexcept final {
    Done(StopTag{}, [] {
    });
  }

 private:
  union {
    Result<V, E> _result;
  };
};

template <>
class ResultCore<void, void> : public CCore {
 public:
  ResultCore() noexcept : CCore{kEmpty} {
  }

  template <typename T, typename Func>
  void Done(T&&, Func&& f) noexcept {
    std::forward<Func>(f)();
    Drop();
  }

  void Drop() noexcept final {
#ifdef YACLIB_LOG_DEBUG
    SetResult();
#else
    _caller->DecRef();
    DecRef();
#endif
  }
};

extern template class ResultCore<void, StopError>;

template <typename V, typename E>
using ResultCorePtr = IntrusivePtr<ResultCore<V, E>>;

}  // namespace yaclib::detail
