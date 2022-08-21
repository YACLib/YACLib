#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/exe/inline.hpp>

#include <utility>

namespace test {

template <bool Inline, typename Future, typename Func>
auto InlineThen(Future&& future, Func&& f) {
  if constexpr (Inline) {
    return std::move(future).ThenInline(std::forward<Func>(f));
  } else {
    // cast to Future because FutureOn with Inline executor is error prone
    return std::move(future).Then(yaclib::MakeInline(), std::forward<Func>(f)).On(nullptr);
  }
}

template <bool Inline, typename V, typename E, typename Func>
void InlineDetach(yaclib::Future<V, E>&& future, Func&& f) {
  if constexpr (Inline) {
    std::move(future).DetachInline(std::forward<Func>(f));
  } else {
    std::move(future).Detach(yaclib::MakeInline(), std::forward<Func>(f));
  }
}

}  // namespace test
