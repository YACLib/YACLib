#pragma once

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/result.hpp>

#include <gtest/gtest.h>

namespace test {

struct NoneManaged {
  using ValueType = void;
  using ErrorType = yaclib::StopError;

  static constexpr yaclib::ConsumePolicy ConsumeP = yaclib::ConsumePolicy::None;
  static constexpr yaclib::CorePolicy CoreP = yaclib::CorePolicy::Managed;

  NoneManaged(size_t count, yaclib::Promise<ValueType, ErrorType> p) : _p{std::move(p)} {
  }

  ~NoneManaged() {
    std::move(_p).Set();
  }

 private:
  yaclib::Promise<ValueType, ErrorType> _p;
};

struct UnorderedManaged {
  using ValueType = void;
  using ErrorType = yaclib::StopError;

  static constexpr yaclib::ConsumePolicy ConsumeP = yaclib::ConsumePolicy::Unordered;
  static constexpr yaclib::CorePolicy CoreP = yaclib::CorePolicy::Managed;

  UnorderedManaged(size_t count, yaclib::Promise<ValueType, ErrorType> p) : _p{std::move(p)} {
  }

  template <typename Result>
  void Consume(Result&& result) {
  }

  ~UnorderedManaged() {
    std::move(_p).Set();
  }

 private:
  yaclib::Promise<ValueType, ErrorType> _p;
};

struct StaticManaged {
  using ValueType = void;
  using ErrorType = yaclib::StopError;

  static constexpr yaclib::ConsumePolicy ConsumeP = yaclib::ConsumePolicy::Static;
  static constexpr yaclib::CorePolicy CoreP = yaclib::CorePolicy::Managed;

  StaticManaged(size_t count, yaclib::Promise<ValueType, ErrorType> p) : _p{std::move(p)} {
  }

  template <size_t Index, typename Result>
  void Consume(Result&& result) {
  }

  ~StaticManaged() {
    std::move(_p).Set();
  }

 private:
  yaclib::Promise<ValueType, ErrorType> _p;
};

struct DynamicManaged {
  using ValueType = void;
  using ErrorType = yaclib::StopError;

  static constexpr yaclib::ConsumePolicy ConsumeP = yaclib::ConsumePolicy::Dynamic;
  static constexpr yaclib::CorePolicy CoreP = yaclib::CorePolicy::Managed;

  DynamicManaged(size_t count, yaclib::Promise<ValueType, ErrorType> p) : _p{std::move(p)} {
  }

  template <typename Result>
  void Consume(size_t index, Result&& result) {
  }

  ~DynamicManaged() {
    std::move(_p).Set();
  }

 private:
  yaclib::Promise<ValueType, ErrorType> _p;
};

struct NoneOwned {
  using ValueType = void;
  using ErrorType = yaclib::StopError;

  static constexpr yaclib::ConsumePolicy ConsumeP = yaclib::ConsumePolicy::None;
  static constexpr yaclib::CorePolicy CoreP = yaclib::CorePolicy::Owned;

  NoneOwned(size_t count, yaclib::Promise<ValueType, ErrorType> p) : _p{std::move(p)} {
  }

  template <typename Core>
  void Register(size_t i, Core& core) {
  }

  ~NoneOwned() {
    std::move(_p).Set();
  }

 private:
  yaclib::Promise<ValueType, ErrorType> _p;
};

struct UnorderedOwned {
  using ValueType = void;
  using ErrorType = yaclib::StopError;

  static constexpr yaclib::ConsumePolicy ConsumeP = yaclib::ConsumePolicy::Unordered;
  static constexpr yaclib::CorePolicy CoreP = yaclib::CorePolicy::Owned;

  UnorderedOwned(size_t count, yaclib::Promise<ValueType, ErrorType> p) : _p{std::move(p)} {
  }

  template <typename Core>
  void Register(size_t i, Core& core) {
  }

  template <typename Core>
  void Consume(Core& core) {
    auto _ = core.Retire();
  }

  ~UnorderedOwned() {
    std::move(_p).Set();
  }

 private:
  yaclib::Promise<ValueType, ErrorType> _p;
};

struct StaticOwned {
  using ValueType = void;
  using ErrorType = yaclib::StopError;

  static constexpr yaclib::ConsumePolicy ConsumeP = yaclib::ConsumePolicy::Static;
  static constexpr yaclib::CorePolicy CoreP = yaclib::CorePolicy::Owned;

  StaticOwned(size_t count, yaclib::Promise<ValueType, ErrorType> p) : _p{std::move(p)} {
  }

  template <typename Core>
  void Register(size_t i, Core& core) {
  }

  template <size_t Index, typename Core>
  void Consume(Core& core) {
    auto _ = core.Retire();
  }

  ~StaticOwned() {
    std::move(_p).Set();
  }

 private:
  yaclib::Promise<ValueType, ErrorType> _p;
};

struct DynamicOwned {
  using ValueType = void;
  using ErrorType = yaclib::StopError;

  static constexpr yaclib::ConsumePolicy ConsumeP = yaclib::ConsumePolicy::Dynamic;
  static constexpr yaclib::CorePolicy CoreP = yaclib::CorePolicy::Owned;

  DynamicOwned(size_t count, yaclib::Promise<ValueType, ErrorType> p) : _p{std::move(p)} {
  }

  template <typename Core>
  void Register(size_t i, Core& core) {
  }

  template <typename Core>
  void Consume(size_t index, Core& core) {
    auto _ = core.Retire();
  }

  ~DynamicOwned() {
    std::move(_p).Set();
  }

 private:
  yaclib::Promise<ValueType, ErrorType> _p;
};

template <typename T>
struct WhenSuite : public testing::Test {
  using Strategy = T;
};

using WhenTypes = ::testing::Types<NoneManaged, UnorderedManaged, StaticManaged, DynamicManaged, NoneOwned,
                                   UnorderedOwned, StaticOwned, DynamicOwned>;

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

}  // namespace test
