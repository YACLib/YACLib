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

template <typename Strategy, typename Core>
YACLIB_INLINE void ConsumeImpl(Strategy& st, Core& core) {
  if constexpr (Strategy::Policy == StrategyPolicy::Owned) {
    st.Consume(core);
  } else {
    st.Consume(core.Retire());
  }
}

template <size_t Index, typename Strategy, typename Core>
YACLIB_INLINE void ConsumeImpl(Strategy& st, Core& core) {
  if constexpr (Strategy::Policy == StrategyPolicy::Owned) {
    st.template Consume<Index>(core);
  } else {
    st.template Consume<Index>(core.Retire());
  }
}

template <typename Strategy, typename Core>
YACLIB_INLINE void ConsumeImpl(Strategy& st, Core& core, size_t index) {
  if constexpr (Strategy::Policy == StrategyPolicy::Owned) {
    st.Consume(index, core);
  } else {
    st.Consume(index, core.Retire());
  }
}

template <size_t Index, typename Strategy, typename Core>
YACLIB_INLINE void Consume(Strategy& st, Core& core) {
  if constexpr (Strategy::Order == StrategyOrder::None) {
    ConsumeImpl(st, core);
  } else if constexpr (Strategy::Order == StrategyOrder::Static) {
    ConsumeImpl<Index>(st, core);
  } else {
    ConsumeImpl(st, core, Index);
  }
}

template <typename Strategy, typename Core>
YACLIB_INLINE void Consume(Strategy& st, Core& core, size_t index) {
  static_assert(Strategy::Order != StrategyOrder::Static);

  if constexpr (Strategy::Order == StrategyOrder::None) {
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

  static constexpr size_t unique_count = std::tuple_size_v<UniqueUniqueCores>;
  static constexpr size_t shared_count = std::tuple_size_v<SharedCores>;
  static constexpr size_t total_count = unique_count + shared_count;
};

template <typename Strategy, typename... Cores>
struct SingleStaticCombinator
  : StaticCombinatorCallback<SingleStaticCombinator<Strategy, Cores...>, head_t<Cores...>, 0> {
  using Base = StaticCombinatorCallback<SingleStaticCombinator<Strategy, Cores...>, head_t<Cores...>, 0>;
  using Core = head_t<Cores...>;

  static_assert(Strategy::Order == StrategyOrder::None);
  static_assert(IsUniqueCore<Core>::Value);
  static_assert((... && std::is_same_v<Core, Cores>));

  using StrategyType = Strategy;

  SingleStaticCombinator(yaclib::Promise<typename Strategy::ValueType, typename Strategy::ErrorType> p)
    : Base{this}, st{sizeof...(Cores), std::move(p)} {
  }

  void SetCore(Core& core, size_t i) {
    if constexpr (Strategy::Policy == StrategyPolicy::Owned) {
      st.Register(i, core);
    }

    if (!core.SetCallback(*this)) {
      Consume<0>(st, core);
      this->DecRef();
    }
  }

  void Set(Cores&... cores) {
    size_t index = 0;
    (..., SetCore(cores, index++));
  }

  Strategy st;
};

template <typename Strategy, typename... Cores>
class StaticCombinator : public IRef {
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
    std::conditional_t<Strategy::Order != StrategyOrder::None,
                       typename OrderedCallbacks<decltype(std::make_index_sequence<sizeof...(Cores)>{})>::Type,
                       UnorderedCallbacks<UniqueUniqueCores, SharedCores>>;

 public:
  using StrategyType = Strategy;

  StaticCombinator(yaclib::Promise<typename Strategy::ValueType, typename Strategy::ErrorType> p)
    : st{sizeof...(Cores), std::move(p)} {
    if constexpr (Strategy::Order != StrategyOrder::None) {
      Init(callbacks);
    } else {
      Init(callbacks.unique_tuple);
      Init(callbacks.shared_tuple);
    }
  }

  template <typename Tuple, std::size_t... Is>
  void InitImpl(Tuple& tuple, std::index_sequence<Is...>) {
    ((std::get<Is>(tuple) = {this}), ...);
  }

  template <typename Tuple>
  void Init(Tuple& tuple) {
    InitImpl(tuple, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
  }

  template <size_t Index, typename Core>
  void SetCore(Core& core) {
    auto& callback = [&]() -> auto& {
      if constexpr (Strategy::Order != StrategyOrder::None) {
        return std::get<Index>(callbacks);
      } else {
        if constexpr (IsSharedCore<Core>::Value) {
          return std::get<translate_index_v<Index, std::tuple<Cores...>, SharedCores>>(callbacks.shared_tuple);
        } else {
          return std::get<index_of_v<Core, UniqueUniqueCores>>(callbacks.unique_tuple);
        }
      }
    }();

    if constexpr (Strategy::Policy == StrategyPolicy::Owned) {
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

  void Set(Cores&... cores) {
    SetImpl(std::make_index_sequence<sizeof...(Cores)>{}, cores...);
  }

  Strategy st;
  Callbacks callbacks;
};

template <typename Strategy, typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type>
class SingleDynamicCombinator
  : public StaticCombinatorCallback<SingleDynamicCombinator<Strategy, Iterator>, typename Value::Core, 0> {
 public:
  using Base = StaticCombinatorCallback<SingleDynamicCombinator<Strategy, Iterator>, typename Value::Core, 0>;
  using Core = typename Value::Core;
  using StrategyType = Strategy;

  static_assert(Strategy::Order == StrategyOrder::None && IsUniqueCore<Core>::Value);

  SingleDynamicCombinator(size_t count, yaclib::Promise<typename Strategy::ValueType, typename Strategy::ErrorType> p)
    : Base{this}, st{count, std::move(p)} {
  }

  void Set(Iterator begin, std::size_t count) {
    for (size_t i = 0; i < count; ++i) {
      auto& core = *begin->GetCore().Release();
      begin++;

      if constexpr (Strategy::Policy == StrategyPolicy::Owned) {
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
class DynamicCombinator : public IRef {
 public:
  using Core = typename Value::Core;
  using StrategyType = Strategy;

  DynamicCombinator(size_t count, yaclib::Promise<typename Strategy::ValueType, typename Strategy::ErrorType> p)
    : st{count, std::move(p)} {
    callbacks = {count, {this}};
  }

  void Set(Iterator begin, std::size_t count) {
    for (size_t i = 0; i < count; ++i) {
      auto& core = *begin->GetCore().Release();
      begin++;

      if constexpr (Strategy::Policy == StrategyPolicy::Owned) {
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

template <typename Strategy, typename... Futures>
auto When(Futures... futures) {
  if constexpr (sizeof...(Futures) == 0) {
    return Future<typename Strategy::ValueType, typename Strategy::ErrorType>{nullptr};
  }
  auto [f, p] = MakeContract<typename Strategy::ValueType, typename Strategy::ErrorType>();

  using FinalCombinator = std::conditional_t<
    CoreSignature<typename Futures::Core...>::total_count == 1 && Strategy::Order == StrategyOrder::None,
    SingleStaticCombinator<Strategy, typename Futures::Core...>, StaticCombinator<Strategy, typename Futures::Core...>>;

  auto combinator = MakeShared<FinalCombinator>(sizeof...(Futures), std::move(p)).Release();
  combinator->Set(*(futures.GetCore().Release())...);
  return std::move(f);
}

template <typename Strategy, typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type>
auto When(Iterator begin, std::size_t count) {
  static_assert(Strategy::Order != StrategyOrder::Static);

  if (count == 0) {
    return Future<typename Strategy::ValueType, typename Strategy::ErrorType>{nullptr};
  }
  auto [f, p] = MakeContract<typename Strategy::ValueType, typename Strategy::ErrorType>();

  using FinalCombinator =
    std::conditional_t<Strategy::Order == StrategyOrder::None && IsUniqueCore<typename Value::Core>::Value,
                       SingleDynamicCombinator<Strategy, Iterator>, DynamicCombinator<Strategy, Iterator>>;

  auto combinator = MakeShared<FinalCombinator>(count, count, std::move(p)).Release();
  combinator->Set(begin, count);
  return std::move(f);
}

}  // namespace yaclib::detail
