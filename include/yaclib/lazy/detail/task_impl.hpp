#pragma once

#include <yaclib/exe/executor.hpp>

namespace yaclib {
namespace detail {

template <typename E, typename Func>
auto Schedule(IExecutor* e, Func&& f) {
  auto* core = detail::MakeCore<detail::CoreType::Run, void, E>(std::forward<Func>(f));
  core->_executor = e;
  using ResultCoreT = typename std::remove_reference_t<decltype(*core)>::Base;
  return Task{IntrusivePtr<ResultCoreT>{NoRefTag{}, core}};
}

template <bool Force = false>
void Run(detail::BaseCore* head, IExecutor& e) noexcept;

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
Task<V, E> Task<V, E>::On(IExecutor& e) && noexcept {
  _core->_executor = &e;
  return {std::move(_core)};
}

template <typename V, typename E>
Task<V, E> Task<V, E>::On(std::nullptr_t) && noexcept {
  _core->_executor = nullptr;
  return {std::move(this->_core)};
}

template <typename V, typename E>
template <typename Func>
auto Task<V, E>::Then(IExecutor& e, Func&& f) && {
  _core->_executor = &e;
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::Lazy>(_core, std::forward<Func>(f));
}

template <typename V, typename E>
template <typename Func>
auto Task<V, E>::ThenInline(Func&& f) && {
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::LazyInline>(_core, std::forward<Func>(f));
}

template <typename V, typename E>
template <typename Func>
auto Task<V, E>::Then(Func&& f) && {
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::Lazy>(_core, std::forward<Func>(f));
}

template <typename V, typename E>
void Task<V, E>::Cancel() && {
  _core->StoreCallback(detail::MakeEmpty(), detail::BaseCore::kWaitDrop);
  detail::Run<true>(_core.Release(), MakeInline(StopTag{}));
}

template <typename V, typename E>
void Task<V, E>::Detach(IExecutor& e) && noexcept {
  _core->StoreCallback(detail::MakeEmpty(), detail::BaseCore::kWaitDrop);
  detail::Run(_core.Release(), e);
}

template <typename V, typename E>
Future<V, E> Task<V, E>::ToFuture() && noexcept {
  detail::Run(_core.Get(), MakeInline());
  _core->_executor = nullptr;
  return {std::move(_core)};
}

template <typename V, typename E>
FutureOn<V, E> Task<V, E>::ToFuture(IExecutor& e) && noexcept {
  YACLIB_WARN(e.Tag() == IExecutor::Type::Inline, "better way is use ToFuture() instead of ToFuture(MakeInline())");
  detail::Run(_core.Get(), e);
  _core->_executor = &e;
  return {std::move(_core)};
}

template <typename V, typename E>
Result<V, E> Task<V, E>::Get(IExecutor& e) && noexcept {  // Stub implementation
  // TODO(MBkkt) make it better: we can remove concurrent atomic changes from here
  return std::move(*this).ToFuture(e).Get();
}

template <typename V, typename E>
detail::ResultCorePtr<V, E>& Task<V, E>::GetCore() noexcept {
  return _core;
}

template <typename V, typename E>
Task<V, E>::Task(detail::ResultCorePtr<V, E> core) noexcept : _core{std::move(core)} {
}

}  // namespace yaclib
