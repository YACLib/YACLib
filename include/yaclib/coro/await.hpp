#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/await_awaiter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename Waited>
struct HandleTrait {
  using Type = void;
};

template <typename V, typename E>
struct HandleTrait<Future<V, E>> {
  using Type = detail::UniqueHandle;
};

template <typename V, typename E>
struct HandleTrait<FutureOn<V, E>> {
  using Type = detail::UniqueHandle;
};

template <typename V, typename E>
struct HandleTrait<Task<V, E>> {
  using Type = detail::UniqueHandle;
};

template <typename V, typename E>
struct HandleTrait<SharedFuture<V, E>> {
  using Type = detail::SharedHandle;
};

template <typename V, typename E>
struct HandleTrait<SharedFutureOn<V, E>> {
  using Type = detail::SharedHandle;
};

template <typename... Waited>
struct MultiAwaitStaticTrait {
  static constexpr size_t SharedCount = TypeCount<detail::SharedHandle, typename HandleTrait<Waited>::Type...>();
  using Return =
    std::conditional_t<SharedCount == 0, detail::MultiAwaitAwaiter<detail::AwaitEvent>,
                       detail::MultiAwaitAwaiter<detail::StaticSharedEvent<detail::AwaitEvent, SharedCount>>>;
};

template <typename Iterator>
struct MultiAwaitDynamicTrait {
  using Handle = typename HandleTrait<typename std::iterator_traits<Iterator>::value_type>::Type;
  static constexpr bool Shared = std::is_same_v<Handle, detail::SharedHandle>;
  using Return = std::conditional_t<Shared, detail::MultiAwaitAwaiter<detail::DynamicSharedEvent<detail::AwaitEvent>>,
                                    detail::MultiAwaitAwaiter<detail::AwaitEvent>>;
};

template <typename V, typename E>
YACLIB_INLINE auto Await(Task<V, E>& task) noexcept {
  YACLIB_ASSERT(task.Valid());
  return detail::TransferAwaiter{UpCast<detail::BaseCore>(*task.GetCore())};
}

template <typename Waited>
YACLIB_INLINE std::enable_if_t<is_waitable_v<Waited>, detail::AwaitAwaiter<typename HandleTrait<Waited>::Type>> Await(
  Waited& waited) noexcept {
  YACLIB_ASSERT(waited.Valid());
  return detail::AwaitAwaiter{waited.GetBaseHandle()};
}

template <typename... Waited>
YACLIB_INLINE std::enable_if_t<(... && is_waitable_v<Waited>), typename MultiAwaitStaticTrait<Waited...>::Return> Await(
  Waited&... waited) noexcept {
  YACLIB_ASSERT(... && waited.Valid());
  return typename MultiAwaitStaticTrait<Waited...>::Return{waited.GetBaseHandle()...};
}

template <typename Iterator>
YACLIB_INLINE std::enable_if_t<is_waitable_v<typename std::iterator_traits<Iterator>::value_type>,
                               typename MultiAwaitDynamicTrait<Iterator>::Return>
Await(Iterator begin, std::size_t count) noexcept {
  return typename MultiAwaitDynamicTrait<Iterator>::Return{begin, count};
}

template <typename Iterator>
YACLIB_INLINE std::enable_if_t<is_waitable_v<typename std::iterator_traits<Iterator>::value_type>,
                               typename MultiAwaitDynamicTrait<Iterator>::Return>
Await(Iterator begin, Iterator end) noexcept {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Await(begin, distance(begin, end))
  return Await(begin, static_cast<std::size_t>(end - begin));
}

template <typename V, typename E>
YACLIB_INLINE auto operator co_await(FutureBase<V, E>&& future) noexcept {
  YACLIB_ASSERT(future.Valid());
  return detail::AwaitSingleAwaiter<false, V, E>{std::move(future.GetCore())};
}

template <typename V, typename E>
YACLIB_INLINE auto operator co_await(const SharedFutureBase<V, E>& future) noexcept {
  YACLIB_ASSERT(future.Valid());
  return detail::AwaitSingleAwaiter<true, V, E>{future.GetCore()};
}

template <typename V, typename E>
YACLIB_INLINE auto operator co_await(Task<V, E>&& task) noexcept {
  YACLIB_ASSERT(task.Valid());
  return detail::TransferSingleAwaiter{std::move(task.GetCore())};
}

}  // namespace yaclib
