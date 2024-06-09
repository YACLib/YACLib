#pragma once

#include <yaclib/algo/detail/promise_core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {
namespace detail {

void Start(BaseCore* head, IExecutor& e) noexcept;
void Start(BaseCore* head) noexcept;

}  // namespace detail

/**
 * Provides a mechanism to schedule the some async operations
 * TODO(MBkkt) add description
 */
template <typename V, typename E>
class Task final {
 public:
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>, "Task cannot be instantiated with same V and E, because it's ambiguous");

  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  Task(Task&& other) noexcept = default;
  Task& operator=(Task&& other) noexcept = default;

  Task() noexcept = default;
  ~Task() noexcept {
    if (Valid()) {
      std::move(*this).Cancel();
    }
  }

  [[nodiscard]] bool Valid() const& noexcept {
    return _core != nullptr;
  }

  /**
   * Check that \ref Result that corresponds to this \ref Task is computed
   *
   * \return false if the \ref Result of this \ref Task is not computed yet, otherwise true
   */
  [[nodiscard]] bool Ready() const& noexcept {
    YACLIB_ASSERT(Valid());
    return !_core->Empty();
  }

  /**
   * Do nothing, just for compatibility with FutureOn
   * TODO(MBkkt) think about force On/Detach/ToFuture:
   *  It's able to set passed executor to previous nullptr/all/head/etc or replace
   */
  Task<V, E> On(std::nullptr_t) && noexcept {
    return {std::move(this->_core)};
  }

  template <typename Func>
  /*Task*/ auto Then(IExecutor& e, Func&& f) && {
    return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::LazyOn>(_core, &e, std::forward<Func>(f));
  }
  template <typename Func>
  /*Task*/ auto ThenInline(Func&& f) && {
    return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::LazyInline>(_core, nullptr,
                                                                                         std::forward<Func>(f));
  }
  template <typename Func>
  /*Task*/ auto Then(Func&& f) && {
    return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::LazyOn>(_core, nullptr,
                                                                                     std::forward<Func>(f));
  }

  void Cancel() && {
    std::move(*this).Detach(MakeInline(StopTag{}));
  }

  void Detach() && noexcept {
    YACLIB_ASSERT(Valid());
    auto* core = _core.Release();
    core->StoreCallback(detail::MakeDrop());
    detail::Start(core);
  }
  void Detach(IExecutor& e) && noexcept {
    YACLIB_ASSERT(Valid());
    auto* core = _core.Release();
    core->StoreCallback(detail::MakeDrop());
    detail::Start(core, e);
  }

  Future<V, E> ToFuture() && noexcept {
    detail::Start(_core.Get());
    return {std::move(_core)};
  }
  FutureOn<V, E> ToFuture(IExecutor& e) && noexcept {
    detail::Start(_core.Get(), e);
    return {std::move(_core)};
  }

  Result<V, E> Get() && noexcept {
    // TODO(MBkkt) make it better: we can remove concurrent atomic changes from here
    return std::move(*this).ToFuture().Get();
  }

  void Touch() & = delete;
  void Touch() const&& = delete;

  const Result<V, E>& Touch() const& noexcept {
    YACLIB_ASSERT(Ready());
    return _core->Get();
  }
  Result<V, E> Touch() && noexcept {
    YACLIB_ASSERT(Ready());
    auto core = std::exchange(_core, nullptr);
    return std::move(core->Get());
  }

  /**
   * Method that get internal Core state
   *
   * \return internal Core state ptr
   */
  [[nodiscard]] detail::ResultCorePtr<V, E>& GetCore() noexcept {
    return _core;
  }
  Task(detail::ResultCorePtr<V, E> core) noexcept : _core{std::move(core)} {
  }

 private:
  detail::ResultCorePtr<V, E> _core;
};

extern template class Task<>;

}  // namespace yaclib
