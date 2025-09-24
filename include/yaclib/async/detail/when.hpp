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

namespace yaclib::detail {

template <typename T>
using IsUniqueCore = detail::IsInstantiationOf<detail::UniqueCore, T>;

template <typename T>
using IsSharedCore = detail::IsInstantiationOf<detail::SharedCore, T>;

static constexpr bool IsOrdered(ConsumePolicy p) {
  return p == ConsumePolicy::Static || p == ConsumePolicy::Dynamic;
}

template <typename Strategy, typename Core>
YACLIB_INLINE void ConsumeImpl(Strategy& st, Core& core) {
  if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
    st.Consume(core);
  } else {
    st.Consume(core.Retire());
  }
}

template <size_t Index, typename Strategy, typename Core>
YACLIB_INLINE void ConsumeImpl(Strategy& st, Core& core) {
  if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
    st.template Consume<Index>(core);
  } else {
    st.template Consume<Index>(core.Retire());
  }
}

template <typename Strategy, typename Core>
YACLIB_INLINE void ConsumeImpl(Strategy& st, Core& core, size_t index) {
  if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
    st.Consume(index, core);
  } else {
    st.Consume(index, core.Retire());
  }
}

template <size_t Index, typename Strategy, typename Core>
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
YACLIB_INLINE void Consume(Strategy& st, Core& core, size_t index) {
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

template <typename Combinator, typename Core, size_t Index>
struct StaticCombinatorCallback : public InlineCore {
  StaticCombinatorCallback() = default;
  StaticCombinatorCallback(Combinator* self) : _self(self) {
  }

  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept override {
    Impl(caller);
    return nullptr;
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept override {
    Impl(caller);
    return yaclib_std::noop_coroutine();
  }
#endif

 private:
  YACLIB_INLINE void Impl(InlineCore& caller) {
    auto& core = DownCast<Core>(caller);
    Consume<Index>(_self->st, core);
    _self->DecRef();
  }

  Combinator* _self;
};

template <typename Combinator, typename Core>
struct DynamicCombinatorCallback : public InlineCore {
  DynamicCombinatorCallback() = default;
  DynamicCombinatorCallback(Combinator* self) : _self(self) {
  }

  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept override {
    Impl(caller);
    return nullptr;
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept override {
    Impl(caller);
    return yaclib_std::noop_coroutine();
  }
#endif

 private:
  YACLIB_INLINE void Impl(InlineCore& caller) {
    auto& core = DownCast<Core>(caller);
    auto index = this - _self->callbacks.data();
    Consume(_self->st, core, index);
    _self->DecRef();
  }

  Combinator* _self;
};

template <typename... Cores>
struct CoreSignature {
  using UniqueUniqueCores = typename Unique<typename Filter<IsUniqueCore, std::tuple<Cores...>>::Type>::Type;
  using SharedCores = typename Filter<IsSharedCore, std::tuple<Cores...>>::Type;

  static constexpr size_t kUniqueCount = std::tuple_size_v<UniqueUniqueCores>;
  static constexpr size_t kSharedCount = std::tuple_size_v<SharedCores>;
  static constexpr size_t kTotalCount = kUniqueCount + kSharedCount;
};

template <typename Strategy, typename... Cores>
struct SingleStaticCombinator
  : StaticCombinatorCallback<SingleStaticCombinator<Strategy, Cores...>, head_t<Cores...>, 0> {
 private:
  using Base = StaticCombinatorCallback<SingleStaticCombinator<Strategy, Cores...>, head_t<Cores...>, 0>;
  using Core = head_t<Cores...>;

  static_assert(!IsOrdered(Strategy::kConsumePolicy));
  static_assert(IsUniqueCore<Core>::Value);
  static_assert((... && std::is_same_v<Core, Cores>));

  void SetCore(Core& core, size_t i) {
    if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
      st.Register(i, core);
    }

    if (!core.SetCallback(*this)) {
      Consume<0>(st, core);
      this->DecRef();
    }
  }

 public:
  SingleStaticCombinator(typename Strategy::PromiseType p) : Base{this}, st{sizeof...(Cores), std::move(p)} {
  }

  void Set(Cores&... cores) {
    size_t index = 0;
    (..., SetCore(cores, index++));
  }

  Strategy st;
};

template <typename Strategy, typename... Cores>
struct StaticCombinator : public IRef {
 private:
  template <typename Sequence>
  struct OrderedCallbacks;

  template <std::size_t... Is>
  struct OrderedCallbacks<std::index_sequence<Is...>> {
    using Type = std::tuple<StaticCombinatorCallback<StaticCombinator, Cores, Is>...>;
  };

  using UniqueUniqueCores = typename CoreSignature<Cores...>::UniqueUniqueCores;
  using SharedCores = typename CoreSignature<Cores...>::SharedCores;

  template <typename UniqueTuple, typename SharedTuple>
  struct UnorderedCallbacks;

  template <typename... UniqueCores, typename... SharedCores>
  struct UnorderedCallbacks<std::tuple<UniqueCores...>, std::tuple<SharedCores...>> {
    std::tuple<StaticCombinatorCallback<StaticCombinator, UniqueCores, 0>...> unique_tuple;
    std::tuple<StaticCombinatorCallback<StaticCombinator, SharedCores, 0>...> shared_tuple;
  };

  using Callbacks =
    std::conditional_t<IsOrdered(Strategy::kConsumePolicy),
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

  template <size_t Index, typename Core>
  auto& GetCallbackHelper() {
    if constexpr (IsOrdered(Strategy::kConsumePolicy)) {
      return std::get<Index>(callbacks);
    } else if constexpr (IsSharedCore<Core>::Value) {
      return std::get<translate_index_v<Index, std::tuple<Cores...>, SharedCores>>(callbacks.shared_tuple);
    } else {
      return std::get<index_of_v<Core, UniqueUniqueCores>>(callbacks.unique_tuple);
    }
  }

  template <size_t Index, typename Core>
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
  StaticCombinator(typename Strategy::PromiseType p) : st{sizeof...(Cores), std::move(p)} {
    if constexpr (IsOrdered(Strategy::kConsumePolicy)) {
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

template <typename Strategy, typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type>
struct SingleDynamicCombinator
  : StaticCombinatorCallback<SingleDynamicCombinator<Strategy, Iterator>, typename Value::Core, 0> {
 private:
  using Base = StaticCombinatorCallback<SingleDynamicCombinator<Strategy, Iterator>, typename Value::Core, 0>;
  using Core = typename Value::Core;

  static_assert(!IsOrdered(Strategy::kConsumePolicy));
  static_assert(IsUniqueCore<Core>::Value);

 public:
  SingleDynamicCombinator(size_t count, typename Strategy::PromiseType p) : Base{this}, st{count, std::move(p)} {
  }

  void Set(Iterator begin, std::size_t count) {
    for (size_t i = 0; i < count; ++i) {
      auto& core = *begin->GetCore().Release();
      begin++;

      if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
        st.Register(i, core);
      }

      if (!core.SetCallback(*this)) {
        Consume(st, core, i);
        this->DecRef();
      }
    }
  }

  Strategy st;
};

template <typename Strategy, typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type>
struct DynamicCombinator : public IRef {
  using Core = typename Value::Core;

  DynamicCombinator(size_t count, typename Strategy::PromiseType p)
    : st{count, std::move(p)}, callbacks{count, {this}} {
  }

  void Set(Iterator begin, std::size_t count) {
    for (size_t i = 0; i < count; ++i) {
      auto& core = *begin->GetCore().Release();
      begin++;

      if constexpr (Strategy::kCorePolicy == CorePolicy::Owned) {
        st.Register(i, core);
      }

      if (!core.SetCallback(callbacks[i])) {
        Consume(st, core, i);
        DecRef();
      }
    }
  }

  Strategy st;
  std::vector<DynamicCombinatorCallback<DynamicCombinator, Core>> callbacks;
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
                                            ResultCore<Value, Error>, InlineCore>>;

    using S = Strategy<F, OutputValue, OutputError, InputCore>;

    using FinalCombinator =
      std::conditional_t<CoreSignature<typename Futures::Core...>::kTotalCount == 1 && !IsOrdered(S::kConsumePolicy),
                         SingleStaticCombinator<S, typename Futures::Core...>,
                         StaticCombinator<S, typename Futures::Core...>>;

    auto combinator = MakeShared<FinalCombinator>(sizeof...(Futures), std::move(p)).Release();
    combinator->Set(*(futures.GetCore().Release())...);
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

  using S = Strategy<F, OutputValue, OutputError, typename Value::Core>;

  static_assert(S::kConsumePolicy != ConsumePolicy::Static);

  using FinalCombinator = std::conditional_t<!IsOrdered(S::kConsumePolicy) && IsUniqueCore<typename Value::Core>::Value,
                                             SingleDynamicCombinator<S, Iterator>, DynamicCombinator<S, Iterator>>;

  auto combinator = MakeShared<FinalCombinator>(count, count, std::move(p)).Release();
  combinator->Set(begin, count);
  return std::move(f);
}

}  // namespace yaclib::detail
