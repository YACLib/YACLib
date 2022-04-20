#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/executor/inline.hpp>

#include <utility>

namespace test {

template <bool Inline, typename V, typename E, typename Functor>
auto InlineThen(yaclib::Future<V, E>&& future, Functor&& f) {
  if constexpr (Inline) {
    return std::move(future).ThenInline(std::forward<Functor>(f));
  } else {
    return std::move(future).Then(yaclib::MakeInline(), std::forward<Functor>(f));
  }
}

template <bool Inline, typename V, typename E, typename Functor>
void InlineDetach(yaclib::Future<V, E>&& future, Functor&& f) {
  if constexpr (Inline) {
    std::move(future).DetachInline(std::forward<Functor>(f));
  } else {
    std::move(future).Detach(yaclib::MakeInline(), std::forward<Functor>(f));
  }
}

}  // namespace test
