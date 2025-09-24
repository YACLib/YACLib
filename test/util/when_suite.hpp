#pragma once

#include <yaclib/async/detail/when.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/result.hpp>

#include <type_traits>

#include <gtest/gtest.h>

namespace test {

template <yaclib::FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
struct NoneManaged {
  using PromiseType = yaclib::Promise<OutputValue, OutputError>;

  static constexpr yaclib::ConsumePolicy kConsumePolicy = yaclib::ConsumePolicy::None;
  static constexpr yaclib::CorePolicy kCorePolicy = yaclib::CorePolicy::Managed;

  NoneManaged(size_t count, PromiseType p) : _p{std::move(p)} {
  }

  ~NoneManaged() {
    std::move(this->_p).Set();
  }

 private:
  PromiseType _p;
};

template <yaclib::FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
struct UnorderedManaged {
  using PromiseType = yaclib::Promise<OutputValue, OutputError>;

  static constexpr yaclib::ConsumePolicy kConsumePolicy = yaclib::ConsumePolicy::Unordered;
  static constexpr yaclib::CorePolicy kCorePolicy = yaclib::CorePolicy::Managed;

  UnorderedManaged(size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <typename Result>
  void Consume(Result&& result) {
  }

  ~UnorderedManaged() {
    std::move(this->_p).Set();
  }

 private:
  PromiseType _p;
};

template <yaclib::FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
struct StaticManaged {
  using PromiseType = yaclib::Promise<OutputValue, OutputError>;

  static constexpr yaclib::ConsumePolicy kConsumePolicy = yaclib::ConsumePolicy::Static;
  static constexpr yaclib::CorePolicy kCorePolicy = yaclib::CorePolicy::Managed;

  StaticManaged(size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <size_t Index, typename Result>
  void Consume(Result&& result) {
  }

  ~StaticManaged() {
    std::move(this->_p).Set();
  }

 private:
  PromiseType _p;
};

template <yaclib::FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
struct DynamicManaged {
  using PromiseType = yaclib::Promise<OutputValue, OutputError>;

  static constexpr yaclib::ConsumePolicy kConsumePolicy = yaclib::ConsumePolicy::Dynamic;
  static constexpr yaclib::CorePolicy kCorePolicy = yaclib::CorePolicy::Managed;

  DynamicManaged(size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <typename Result>
  void Consume(size_t index, Result&& result) {
  }

  ~DynamicManaged() {
    std::move(this->_p).Set();
  }

 private:
  PromiseType _p;
};

template <yaclib::FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
struct NoneOwned {
  using PromiseType = yaclib::Promise<OutputValue, OutputError>;

  static constexpr yaclib::ConsumePolicy kConsumePolicy = yaclib::ConsumePolicy::None;
  static constexpr yaclib::CorePolicy kCorePolicy = yaclib::CorePolicy::Owned;

  NoneOwned(size_t count, PromiseType p) : _p{std::move(p)} {
  }

  void Register(size_t i, InputCore& core) {
    _cores.push_back(&core);
  }

  ~NoneOwned() {
    std::move(this->_p).Set();
    for (size_t i = 0; i < _cores.size(); ++i) {
      _cores[i]->DecRef();
    }
  }

 private:
  std::vector<InputCore*> _cores;
  PromiseType _p;
};

template <yaclib::FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
struct UnorderedOwned {
  using PromiseType = yaclib::Promise<OutputValue, OutputError>;

  static constexpr yaclib::ConsumePolicy kConsumePolicy = yaclib::ConsumePolicy::Unordered;
  static constexpr yaclib::CorePolicy kCorePolicy = yaclib::CorePolicy::Owned;

  UnorderedOwned(size_t count, PromiseType p) : _p{std::move(p)} {
  }

  void Register(size_t i, InputCore& core) {
  }

  void Consume(InputCore& core) {
    core.DecRef();
  }

  ~UnorderedOwned() {
    std::move(this->_p).Set();
  }

 private:
  PromiseType _p;
};

template <yaclib::FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
struct StaticOwned {
  using PromiseType = yaclib::Promise<OutputValue, OutputError>;

  static constexpr yaclib::ConsumePolicy kConsumePolicy = yaclib::ConsumePolicy::Static;
  static constexpr yaclib::CorePolicy kCorePolicy = yaclib::CorePolicy::Owned;

  StaticOwned(size_t count, PromiseType p) : _p{std::move(p)} {
  }

  void Register(size_t i, InputCore& core) {
  }

  template <size_t Index>
  void Consume(InputCore& core) {
    core.DecRef();
  }

  ~StaticOwned() {
    std::move(this->_p).Set();
  }

 private:
  PromiseType _p;
};

template <yaclib::FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
struct DynamicOwned {
  using PromiseType = yaclib::Promise<OutputValue, OutputError>;

  static constexpr yaclib::ConsumePolicy kConsumePolicy = yaclib::ConsumePolicy::Dynamic;
  static constexpr yaclib::CorePolicy kCorePolicy = yaclib::CorePolicy::Owned;

  DynamicOwned(size_t count, PromiseType p) : _p{std::move(p)} {
  }

  void Register(size_t i, InputCore& core) {
  }

  void Consume(size_t index, InputCore& core) {
    core.DecRef();
  }

  ~DynamicOwned() {
    std::move(this->_p).Set();
  }

 private:
  PromiseType _p;
};

struct NoneManagedTag {};
struct UnorderedManagedTag {};
struct StaticManagedTag {};
struct DynamicManagedTag {};
struct NoneOwnedTag {};
struct UnorderedOwnedTag {};
struct StaticOwnedTag {};
struct DynamicOwnedTag {};

template <typename T>
struct WhenSuite : public testing::Test {
  using Strategy = T;
};

using WhenTypes = ::testing::Types<NoneManagedTag, UnorderedManagedTag, StaticManagedTag, DynamicManagedTag,
                                   NoneOwnedTag, UnorderedOwnedTag, StaticOwnedTag, DynamicOwnedTag>;

struct WhenNames {
  template <typename T>
  static std::string GetName(int i) {
    switch (i) {
      case 0:
        return "NoneManaged";
      case 1:
        return "UnorderedManaged";
      case 2:
        return "StaticManaged";
      case 3:
        return "DynamicManaged";
      case 4:
        return "NoneOwned";
      case 5:
        return "UnorderedOwned";
      case 6:
        return "StaticOwned";
      case 7:
        return "DynamicOwned";
      default:
        return "unknown";
    }
  }
};

TYPED_TEST_SUITE(WhenSuite, WhenTypes, WhenNames);

template <typename StrategyT, typename... Futures>
auto CallWhen(Futures... futures) {
  static constexpr auto F = yaclib::FailPolicy::None;
  using V = void;
  using E = yaclib::StopError;

  if constexpr (std::is_same_v<StrategyT, NoneManagedTag>) {
    return yaclib::detail::When<NoneManaged, F, V, E>(std::move(futures)...);
  } else if constexpr (std::is_same_v<StrategyT, UnorderedManagedTag>) {
    return yaclib::detail::When<UnorderedManaged, F, V, E>(std::move(futures)...);
  } else if constexpr (std::is_same_v<StrategyT, StaticManagedTag>) {
    return yaclib::detail::When<StaticManaged, F, V, E>(std::move(futures)...);
  } else if constexpr (std::is_same_v<StrategyT, DynamicManagedTag>) {
    return yaclib::detail::When<DynamicManaged, F, V, E>(std::move(futures)...);
  } else if constexpr (std::is_same_v<StrategyT, NoneOwnedTag>) {
    return yaclib::detail::When<NoneOwned, F, V, E>(std::move(futures)...);
  } else if constexpr (std::is_same_v<StrategyT, UnorderedOwnedTag>) {
    return yaclib::detail::When<UnorderedOwned, F, V, E>(std::move(futures)...);
  } else if constexpr (std::is_same_v<StrategyT, StaticOwnedTag>) {
    return yaclib::detail::When<StaticOwned, F, V, E>(std::move(futures)...);
  } else if constexpr (std::is_same_v<StrategyT, DynamicOwnedTag>) {
    return yaclib::detail::When<DynamicOwned, F, V, E>(std::move(futures)...);
  } else {
    return nullptr;
  }
}

template <typename StrategyT, typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type>
auto CallWhen(Iterator begin, Iterator end) {
  static constexpr auto F = yaclib::FailPolicy::None;
  using V = void;
  using E = yaclib::StopError;

  if constexpr (std::is_same_v<StrategyT, NoneManagedTag>) {
    return yaclib::detail::When<NoneManaged, F, V, E>(begin, end);
  } else if constexpr (std::is_same_v<StrategyT, UnorderedManagedTag>) {
    return yaclib::detail::When<UnorderedManaged, F, V, E>(begin, end);
  } else if constexpr (std::is_same_v<StrategyT, StaticManagedTag>) {
    return yaclib::detail::When<StaticManaged, F, V, E>(begin, end);
  } else if constexpr (std::is_same_v<StrategyT, DynamicManagedTag>) {
    return yaclib::detail::When<DynamicManaged, F, V, E>(begin, end);
  } else if constexpr (std::is_same_v<StrategyT, NoneOwnedTag>) {
    return yaclib::detail::When<NoneOwned, F, V, E>(begin, end);
  } else if constexpr (std::is_same_v<StrategyT, UnorderedOwnedTag>) {
    return yaclib::detail::When<UnorderedOwned, F, V, E>(begin, end);
  } else if constexpr (std::is_same_v<StrategyT, StaticOwnedTag>) {
    return yaclib::detail::When<StaticOwned, F, V, E>(begin, end);
  } else if constexpr (std::is_same_v<StrategyT, DynamicOwnedTag>) {
    return yaclib::detail::When<DynamicOwned, F, V, E>(begin, end);
  } else {
    return nullptr;
  }
}

template <typename StrategyT, typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type>
auto CallWhen(Iterator begin, size_t count) {
  static constexpr auto F = yaclib::FailPolicy::None;
  using V = void;
  using E = yaclib::StopError;

  if constexpr (std::is_same_v<StrategyT, NoneManagedTag>) {
    return yaclib::detail::When<NoneManaged, F, V, E>(begin, count);
  } else if constexpr (std::is_same_v<StrategyT, UnorderedManagedTag>) {
    return yaclib::detail::When<UnorderedManaged, F, V, E>(begin, count);
  } else if constexpr (std::is_same_v<StrategyT, StaticManagedTag>) {
    return yaclib::detail::When<StaticManaged, F, V, E>(begin, count);
  } else if constexpr (std::is_same_v<StrategyT, DynamicManagedTag>) {
    return yaclib::detail::When<DynamicManaged, F, V, E>(begin, count);
  } else if constexpr (std::is_same_v<StrategyT, NoneOwnedTag>) {
    return yaclib::detail::When<NoneOwned, F, V, E>(begin, count);
  } else if constexpr (std::is_same_v<StrategyT, UnorderedOwnedTag>) {
    return yaclib::detail::When<UnorderedOwned, F, V, E>(begin, count);
  } else if constexpr (std::is_same_v<StrategyT, StaticOwnedTag>) {
    return yaclib::detail::When<StaticOwned, F, V, E>(begin, count);
  } else if constexpr (std::is_same_v<StrategyT, DynamicOwnedTag>) {
    return yaclib::detail::When<DynamicOwned, F, V, E>(begin, count);
  } else {
    return nullptr;
  }
}

template <typename StrategyT>
static constexpr bool IsStaticStrategy =
  std::is_same_v<StrategyT, StaticManagedTag> || std::is_same_v<StrategyT, StaticOwnedTag>;

}  // namespace test
