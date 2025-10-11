#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/algo/detail/unique_core.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/util/cast.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>
#include <yaclib/util/type_traits.hpp>

#include <tuple>
#include <vector>

namespace yaclib::when {

template <typename... Futures>
YACLIB_INLINE void CheckSameError() {
  static_assert(sizeof...(Futures) > 0);
  using Error = typename head_t<Futures...>::Core::Error;
  static_assert((... && std::is_same_v<Error, typename Futures::Core::Error>),
                "All futures need to have the same error type");
}

template <typename T>
using IsUniqueCore = detail::IsInstantiationOf<detail::UniqueCore, T>;

template <typename T>
using IsSharedCore = detail::IsInstantiationOf<detail::SharedCore, T>;

template <ConsumePolicy P>
inline constexpr bool kIsOrdered = P == ConsumePolicy::Static || P == ConsumePolicy::Dynamic;

inline constexpr std::size_t kDynamicTag = std::numeric_limits<std::size_t>::max();

template <typename Strategy, typename Core>
YACLIB_INLINE void ConsumeImpl(Strategy& st, Core& core) {
  if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
    st.Consume(core);
  } else {
    st.Consume(core.Retire());
  }
}

template <std::size_t Index, typename Strategy, typename Core>
YACLIB_INLINE void ConsumeImpl(Strategy& st, Core& core) {
  if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
    st.template Consume<Index>(core);
  } else {
    st.template Consume<Index>(core.Retire());
  }
}

template <typename Strategy, typename Core>
YACLIB_INLINE void ConsumeImpl(Strategy& st, Core& core, std::size_t index) {
  if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
    st.Consume(index, core);
  } else {
    st.Consume(index, core.Retire());
  }
}

template <std::size_t Index, typename Strategy, typename Core>
YACLIB_INLINE void Consume(Strategy& st, Core& core) {
  if constexpr (Strategy::kConsumePolicy == ConsumePolicy::None) {
    if constexpr (Strategy::kCorePolicy == CorePolicy::Managed) {
      core.DecRef();
    }
  } else if constexpr (Strategy::kConsumePolicy == ConsumePolicy::Unordered) {
    ConsumeImpl(st, core);
  } else if constexpr (Strategy::kConsumePolicy == ConsumePolicy::Static) {
    ConsumeImpl<Index>(st, core);
  } else {
    ConsumeImpl(st, core, Index);
  }
}

template <typename Strategy, typename Core>
YACLIB_INLINE void Consume(Strategy& st, Core& core, std::size_t index) {
  static_assert(Strategy::kConsumePolicy != ConsumePolicy::Static);

  if constexpr (Strategy::kConsumePolicy == ConsumePolicy::None) {
    if constexpr (Strategy::kCorePolicy == CorePolicy::Managed) {
      core.DecRef();
    }
  } else if constexpr (Strategy::kConsumePolicy == ConsumePolicy::Unordered) {
    ConsumeImpl(st, core);
  } else {
    ConsumeImpl(st, core, index);
  }
}

template <typename Combinator, typename Core, std::size_t Index>
struct CombinatorCallback final : detail::InlineCore {
  CombinatorCallback(Combinator* self = nullptr) : _self{self} {
  }

  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept final {
    Impl(caller);
    return nullptr;
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {
    Impl(caller);
    return yaclib_std::noop_coroutine();
  }
#endif

 private:
  YACLIB_INLINE void Impl(InlineCore& caller) {
    auto& core = DownCast<Core>(caller);
    if constexpr (Index == kDynamicTag) {
      auto index = this - _self->callbacks.data();
      Consume(_self->st, core, index);
    } else {
      Consume<Index>(_self->st, core);
    }
    _self->DecRef();
  }

  Combinator* _self;
};

template <typename... Cores>
struct CoreSignature {
  using UniqueUniqueCores = typename Unique<typename Filter<IsUniqueCore, std::tuple<Cores...>>::Type>::Type;
  using SharedCores = typename Filter<IsSharedCore, std::tuple<Cores...>>::Type;

  static constexpr std::size_t kUniqueCount = std::tuple_size_v<UniqueUniqueCores>;
  static constexpr std::size_t kSharedCount = std::tuple_size_v<SharedCores>;
  static constexpr std::size_t kTotalCount = kUniqueCount + kSharedCount;
};

template <typename Strategy, typename Core>
struct SingleCombinator : detail::InlineCore {
  SingleCombinator(std::size_t count, typename Strategy::PromiseType p) : st{count, std::move(p)} {
  }

  // Static case Set
  template <typename... Cores>
  void Set(Cores&... cores) {
    static_assert((... && std::is_same_v<Core, Cores>));
    std::size_t index = 0;
    (..., SetCore(cores, index++));
  }

  // Dynamic case Set
  template <typename Iterator, typename = typename std::iterator_traits<Iterator>::value_type>
  void Set(Iterator begin, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
      auto& core = *begin->GetCore().Release();

      if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
        st.Register(i, core);
      }

      if (!core.SetCallback(*this)) {
        Consume(st, core, i);
        DecRef();
      }

      ++begin;
    }
  }

  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept final {
    Impl(caller);
    return nullptr;
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {
    Impl(caller);
    return yaclib_std::noop_coroutine();
  }
#endif

 private:
  void SetCore(Core& core, std::size_t i) {
    if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
      st.Register(i, core);
    }

    if (!core.SetCallback(*this)) {
      Consume<0>(st, core);
      DecRef();
    }
  }

  YACLIB_INLINE void Impl(InlineCore& caller) {
    auto& core = DownCast<Core>(caller);
    Consume<0>(st, core);
    DecRef();
  }

  Strategy st;
};

template <typename Strategy, typename... Cores>
struct StaticCombinator : IRef {
 private:
  template <typename Sequence>
  struct OrderedCallbacks;

  template <std::size_t... Is>
  struct OrderedCallbacks<std::index_sequence<Is...>> {
    using Type = std::tuple<CombinatorCallback<StaticCombinator, Cores, Is>...>;
  };

  using UniqueUniqueCores = typename CoreSignature<Cores...>::UniqueUniqueCores;
  using SharedCores = typename CoreSignature<Cores...>::SharedCores;

  template <typename UniqueTuple, typename SharedTuple>
  struct UnorderedCallbacks;

  template <typename... UniqueCores, typename... SharedCores>
  struct UnorderedCallbacks<std::tuple<UniqueCores...>, std::tuple<SharedCores...>> {
    std::tuple<CombinatorCallback<StaticCombinator, UniqueCores, 0>...> unique_tuple;
    std::tuple<CombinatorCallback<StaticCombinator, SharedCores, 0>...> shared_tuple;
  };

  using Callbacks =
    std::conditional_t<kIsOrdered<Strategy::kConsumePolicy>,
                       typename OrderedCallbacks<decltype(std::make_index_sequence<sizeof...(Cores)>{})>::Type,
                       UnorderedCallbacks<UniqueUniqueCores, SharedCores>>;

  template <typename Tuple, std::size_t... Is>
  void InitImpl(Tuple& tuple, std::index_sequence<Is...>) {
    ((std::get<Is>(tuple) = {this}), ...);
  }

  template <typename Tuple>
  void Init(Tuple& tuple) {
    InitImpl(tuple, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
  }

  template <std::size_t Index, typename Core>
  auto& GetCallbackHelper() {
    if constexpr (kIsOrdered<Strategy::kConsumePolicy>) {
      return std::get<Index>(callbacks);
    } else if constexpr (IsSharedCore<Core>::Value) {
      return std::get<translate_index_v<Index, std::tuple<Cores...>, SharedCores>>(callbacks.shared_tuple);
    } else {
      return std::get<index_of_v<Core, UniqueUniqueCores>>(callbacks.unique_tuple);
    }
  }

  template <std::size_t Index, typename Core>
  void SetCore(Core& core) {
    auto& callback = GetCallbackHelper<Index, Core>();

    if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
      st.Register(Index, core);
    }

    if (!core.SetCallback(callback)) {
      Consume<Index>(st, core);
      DecRef();
    }
  }

  template <std::size_t... Is>
  void SetImpl(std::index_sequence<Is...>, Cores&... cores) {
    (SetCore<Is>(cores), ...);
  }

 public:
  StaticCombinator(std::size_t count, typename Strategy::PromiseType p) : st{count, std::move(p)} {
    if constexpr (kIsOrdered<Strategy::kConsumePolicy>) {
      Init(callbacks);
    } else {
      Init(callbacks.unique_tuple);
      Init(callbacks.shared_tuple);
    }
  }

  void Set(Cores&... cores) {
    SetImpl(std::make_index_sequence<sizeof...(Cores)>{}, cores...);
  }

  Strategy st;
  Callbacks callbacks;
};

template <typename Strategy, typename Core>
struct DynamicCombinator : IRef {
  DynamicCombinator(std::size_t count, typename Strategy::PromiseType p)
    : st{count, std::move(p)}, callbacks{count, {this}} {
  }

  template <typename Iterator>
  void Set(Iterator begin, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
      auto& core = *begin->GetCore().Release();

      if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
        st.Register(i, core);
      }

      if (!core.SetCallback(callbacks[i])) {
        Consume(st, core, i);
        DecRef();
      }

      ++begin;
    }
  }

  Strategy st;
  std::vector<CombinatorCallback<DynamicCombinator, Core, kDynamicTag>> callbacks;
};

template <template <FailPolicy, typename...> typename Strategy, FailPolicy F, typename OutputValue,
          typename OutputError, typename... Futures>
auto When(Futures... futures) {
  if constexpr (sizeof...(Futures) == 0) {
    return Future<OutputValue, OutputError>{nullptr};
  } else {
    auto [f, p] = MakeContract<OutputValue, OutputError>();

    using Head = typename head_t<Futures...>::Core;
    using Value = typename Head::Value;
    using Error = typename Head::Error;

    using InputCore =
      std::conditional_t<(... && std::is_same_v<Head, typename Futures::Core>), Head,
                         std::conditional_t<(... && (std::is_same_v<Value, typename Futures::Core::Value> &&
                                                     std::is_same_v<Error, typename Futures::Core::Error>)),
                                            detail::ResultCore<Value, Error>, detail::InlineCore>>;

    using S = Strategy<F, OutputValue, OutputError, InputCore>;

    using FinalCombinator =
      std::conditional_t<CoreSignature<typename Futures::Core...>::kTotalCount == 1 && !kIsOrdered<S::kConsumePolicy>,
                         SingleCombinator<S, head_t<typename Futures::Core...>>,
                         StaticCombinator<S, typename Futures::Core...>>;

    auto* combinator = MakeShared<FinalCombinator>(sizeof...(Futures), sizeof...(Futures), std::move(p)).Release();
    combinator->Set(*futures.GetCore().Release()...);
    return std::move(f);
  }
}

template <template <FailPolicy, typename...> typename Strategy, FailPolicy F, typename OutputValue,
          typename OutputError, typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type>
auto When(Iterator begin, std::size_t count) {
  if (count == 0) {
    return Future<OutputValue, OutputError>{nullptr};
  }

  auto [f, p] = MakeContract<OutputValue, OutputError>();

  using Core = typename Value::Core;
  using S = Strategy<F, OutputValue, OutputError, Core>;

  static_assert(S::kConsumePolicy != ConsumePolicy::Static);

  using FinalCombinator = std::conditional_t<!kIsOrdered<S::kConsumePolicy> && IsUniqueCore<Core>::Value,
                                             SingleCombinator<S, Core>, DynamicCombinator<S, Core>>;

  auto* combinator = MakeShared<FinalCombinator>(count, count, std::move(p)).Release();
  combinator->Set(begin, count);
  return std::move(f);
}

}  // namespace yaclib::when
