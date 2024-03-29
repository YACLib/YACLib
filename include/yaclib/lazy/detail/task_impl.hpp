#pragma once

#include <yaclib/algo/detail/promise_core.hpp>
#include <yaclib/exe/executor.hpp>

namespace yaclib {
namespace detail {

void Start(BaseCore* head, IExecutor& e) noexcept;
void Start(BaseCore* head) noexcept;

template <typename V, typename E, typename Func>
YACLIB_INLINE auto Schedule(IExecutor& e, Func&& f) {
  auto* core = [&] {
    if constexpr (std::is_same_v<V, Unit>) {
      return MakeCore<CoreType::Run, true, void, E>(std::forward<Func>(f));
    } else {
      return MakeUnique<PromiseCore<V, E, Func&&>>(std::forward<Func>(f)).Release();
    }
  }();
  e.IncRef();
  core->_executor.Reset(NoRefTag{}, &e);
  using ResultCoreT = typename std::remove_reference_t<decltype(*core)>::Base;
  return Task{IntrusivePtr<ResultCoreT>{NoRefTag{}, core}};
}

}  // namespace detail

template <typename V, typename E>
Task<V, E>::~Task() noexcept {
  if (Valid()) {
    std::move(*this).Cancel();
  }
}

template <typename V, typename E>
bool Task<V, E>::Valid() const& noexcept {
  return _core != nullptr;
}

template <typename V, typename E>
Task<V, E> Task<V, E>::On(std::nullptr_t) && noexcept {
  return {std::move(this->_core)};
}

template <typename V, typename E>
template <typename Func>
auto Task<V, E>::Then(IExecutor& e, Func&& f) && {
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::LazyOn>(_core, &e, std::forward<Func>(f));
}

template <typename V, typename E>
template <typename Func>
auto Task<V, E>::ThenInline(Func&& f) && {
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::LazyInline>(_core, nullptr,
                                                                                       std::forward<Func>(f));
}

template <typename V, typename E>
template <typename Func>
auto Task<V, E>::Then(Func&& f) && {
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::LazyOn>(_core, nullptr,
                                                                                   std::forward<Func>(f));
}

template <typename V, typename E>
void Task<V, E>::Cancel() && {
  std::move(*this).Detach(MakeInline(StopTag{}));
}

template <typename V, typename E>
void Task<V, E>::Detach() && noexcept {
  auto* core = _core.Release();
  core->StoreCallback(detail::MakeDrop());
  detail::Start(core);
}

template <typename V, typename E>
void Task<V, E>::Detach(IExecutor& e) && noexcept {
  auto* core = _core.Release();
  core->StoreCallback(detail::MakeDrop());
  detail::Start(core, e);
}

template <typename V, typename E>
Future<V, E> Task<V, E>::ToFuture() && noexcept {
  detail::Start(_core.Get());
  return {std::move(_core)};
}

template <typename V, typename E>
FutureOn<V, E> Task<V, E>::ToFuture(IExecutor& e) && noexcept {
  YACLIB_WARN(e.Tag() == IExecutor::Type::Inline, "better way is use ToFuture() instead of ToFuture(MakeInline())");
  detail::Start(_core.Get(), e);
  return {std::move(_core)};
}

template <typename V, typename E>
Result<V, E> Task<V, E>::Get() && noexcept {  // Stub implementation
  // TODO(MBkkt) make it better: we can remove concurrent atomic changes from here
  return std::move(*this).ToFuture().Get();
}

template <typename V, typename E>
detail::ResultCorePtr<V, E>& Task<V, E>::GetCore() noexcept {
  return _core;
}

template <typename V, typename E>
Task<V, E>::Task(detail::ResultCorePtr<V, E> core) noexcept : _core{std::move(core)} {
}

}  // namespace yaclib
