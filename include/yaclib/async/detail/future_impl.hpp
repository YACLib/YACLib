#pragma once

#include <yaclib/algo/detail/core.hpp>
#include <yaclib/async/wait.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/result.hpp>

namespace yaclib {

template <typename V, typename E>
FutureBase<V, E>::~FutureBase() noexcept {
  if (Valid()) {
    std::move(*this).Detach();
  }
}

template <typename V, typename E>
bool FutureBase<V, E>::Valid() const& noexcept {
  return _core != nullptr;
}

template <typename V, typename E>
bool FutureBase<V, E>::Ready() const& noexcept {
  return !_core->Empty();
}

template <typename V, typename E>
const Result<V, E>* FutureBase<V, E>::Get() const& noexcept {
  if (Ready()) {  // TODO(MBkkt) Maybe we want likely
    return &_core->Get();
  }
  return nullptr;
}

template <typename V, typename E>
Result<V, E> FutureBase<V, E>::Get() && noexcept {
  Wait(*this);
  auto core = std::exchange(_core, nullptr);
  return std::move(core->Get());
}

template <typename V, typename E>
const Result<V, E>& FutureBase<V, E>::Touch() const& noexcept {
  YACLIB_ERROR(!Ready(), "Try to touch result of not ready Future");
  return _core->Get();
}

template <typename V, typename E>
Result<V, E> FutureBase<V, E>::Touch() && noexcept {
  YACLIB_ERROR(!Ready(), "Try to touch result of not ready Future");
  auto core = std::exchange(_core, nullptr);
  return std::move(core->Get());
}

template <typename V, typename E>
template <typename Func>
auto FutureBase<V, E>::Then(IExecutor& e, Func&& f) && {
  YACLIB_WARN(e.Tag() == IExecutor::Type::Inline,
              "better way is use ThenInline(...) instead of Then(MakeInline(), ...)");
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::On>(_core, &e, std::forward<Func>(f));
}

template <typename V, typename E>
void FutureBase<V, E>::Detach() && noexcept {
  auto* core = _core.Release();
  // TODO if use SetCallback it will single virtual call instead of two
  core->CallInline(detail::MakeDrop());
}

template <typename V, typename E>
template <typename Func>
void FutureBase<V, E>::DetachInline(Func&& f) && {
  detail::SetCallback<detail::CoreType::Detach, detail::CallbackType::Inline>(_core, nullptr, std::forward<Func>(f));
}

template <typename V, typename E>
template <typename Func>
void FutureBase<V, E>::Detach(IExecutor& e, Func&& f) && {
  YACLIB_WARN(e.Tag() == IExecutor::Type::Inline,
              "better way is use DetachInline(...) instead of Detach(MakeInline(), ...)");
  detail::SetCallback<detail::CoreType::Detach, detail::CallbackType::On>(_core, &e, std::forward<Func>(f));
}

template <typename V, typename E>
FutureBase<V, E>::FutureBase(detail::ResultCorePtr<V, E> core) noexcept : _core{std::move(core)} {
}

template <typename V, typename E>
detail::ResultCorePtr<V, E>& FutureBase<V, E>::GetCore() noexcept {
  return _core;
}

template <typename V, typename E>
template <typename Func>
auto Future<V, E>::ThenInline(Func&& f) && {
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::Inline>(this->_core, nullptr,
                                                                                   std::forward<Func>(f));
}

template <typename V, typename E>
Future<V, E> FutureOn<V, E>::On(std::nullptr_t) && noexcept {
  return {std::move(this->_core)};
}

template <typename V, typename E>
template <typename Func>
auto FutureOn<V, E>::ThenInline(Func&& f) && {
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::InlineOn>(this->_core, nullptr,
                                                                                     std::forward<Func>(f));
}

template <typename V, typename E>
template <typename Func>
auto FutureOn<V, E>::Then(Func&& f) && {
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::On>(this->_core, nullptr,
                                                                               std::forward<Func>(f));
}

template <typename V, typename E>
template <typename Func>
void FutureOn<V, E>::Detach(Func&& f) && {
  detail::SetCallback<detail::CoreType::Detach, detail::CallbackType::On>(this->_core, nullptr, std::forward<Func>(f));
}

}  // namespace yaclib
