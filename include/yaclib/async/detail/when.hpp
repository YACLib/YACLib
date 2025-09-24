#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/algo/detail/unique_core.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/util/cast.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

#include <tuple>
#include <vector>

namespace yaclib::detail {

template <typename Strategy, typename Core>
YACLIB_INLINE void Consume(Strategy& st, Core& core, size_t index) {
  if constexpr (Strategy::Owner) {
    if constexpr (Strategy::Ordered) {
      st.Consume(index);
    } else {
      st.Consume();
    }
  } else {
    if constexpr (Strategy::Ordered) {
      if (core.GetRef() == 1) {
        st.Consume(index, std::move(core.Get()));
      } else {
        st.Consume(index, std::as_const(core.Get()));
      }
    } else {
      if (core.GetRef() == 1) {
        st.Consume(std::move(core.Get()));
      } else {
        st.Consume(std::as_const(core.Get()));
      }
    }
    core.DecRef();
  }
}

template <typename Combinator, typename Core, size_t Index>
struct StaticCombinatorCallback : public InlineCore {
  StaticCombinatorCallback() = default;
  StaticCombinatorCallback(Combinator* self) : _self(self) {
  }

  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept override {
    auto& core = DownCast<Core>(caller);
    Consume(_self->st, core, Index);
    _self->DecRef();
    return nullptr;
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept override {
    auto& core = DownCast<Core>(caller);
    Consume(_self->st, core, Index);
    _self->DecRef();
    return yaclib_std::noop_coroutine();
  }
#endif

 private:
  Combinator* _self;
};

template <typename Combinator, typename Core>
struct DynamicCombinatorCallback : public InlineCore {
  DynamicCombinatorCallback() = default;
  DynamicCombinatorCallback(Combinator* self) : _self(self) {
  }

  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept override {
    auto& core = DownCast<Core>(caller);
    auto index = this - _self->callbacks.data();
    Consume(_self->st, core, index);
    _self->DecRef();
    return nullptr;
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept override {
    auto& core = DownCast<Core>(caller);
    auto index = this - _self->callbacks.data();
    Consume(_self->st, core, index);
    _self->DecRef();
    return yaclib_std::noop_coroutine();
  }
#endif

 private:
  Combinator* _self;
};

template <typename Strategy, typename... Cores>
class StaticCombinator : public IRef {
  template <typename Sequence>
  struct CallbacksType;

  template <std::size_t... Is>
  struct CallbacksType<std::index_sequence<Is...>> {
    using Type = std::tuple<StaticCombinatorCallback<StaticCombinator, Cores, Is>...>;
  };

 public:
  using StrategyType = Strategy;

  StaticCombinator(yaclib::Promise<typename Strategy::ValueType, typename Strategy::ErrorType> p)
    : st{sizeof...(Cores), std::move(p)} {
  }

  template <size_t Index, typename Core>
  void SetCore(Core& core) {
    auto& callback = std::get<Index>(callbacks);
    callback = {this};

    if constexpr (StrategyType::Owner) {
      st.Register(Index, core);
    }

    if (!core.SetCallback(callback)) {
      Consume(st, core, Index);
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
  typename CallbacksType<decltype(std::make_index_sequence<sizeof...(Cores)>{})>::Type callbacks;
};

template <typename Strategy, typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type>
class DynamicCombinator : public IRef {
 public:
  using Core = std::remove_reference_t<decltype(*std::declval<Value>().GetCore())>;
  using StrategyType = Strategy;

  DynamicCombinator(size_t count, yaclib::Promise<typename Strategy::ValueType, typename Strategy::ErrorType> p)
    : st{count, std::move(p)} {
    callbacks.reserve(count);
  }

  void Set(Iterator begin, std::size_t count) {
    for (size_t i = 0; i < count; ++i) {
      auto& core = *begin->GetCore().Release();
      begin++;

      callbacks.push_back({this});

      if constexpr (StrategyType::Owner) {
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

template <typename Strategy, typename... Cores>
auto When(Cores&... cores) {
  if constexpr (sizeof...(Cores) == 0) {
    return Future<typename Strategy::ValueType, typename Strategy::ErrorType>{nullptr};
  }
  auto [f, p] = MakeContract<typename Strategy::ValueType, typename Strategy::ErrorType>();
  auto combinator = MakeShared<StaticCombinator<Strategy, Cores...>>(sizeof...(Cores), std::move(p)).Release();
  combinator->Set(cores...);
  return std::move(f);
}

template <typename Strategy, typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type>
auto When(Iterator begin, std::size_t count) {
  if (count == 0) {
    return Future<typename Strategy::ValueType, typename Strategy::ErrorType>{nullptr};
  }
  auto [f, p] = MakeContract<typename Strategy::ValueType, typename Strategy::ErrorType>();
  auto combinator = MakeShared<DynamicCombinator<Strategy, Iterator>>(count, count, std::move(p)).Release();
  combinator->Set(begin, count);
  return std::move(f);
}

}  // namespace yaclib::detail
