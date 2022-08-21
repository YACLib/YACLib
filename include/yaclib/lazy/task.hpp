#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

/**
 * Provides a mechanism to schedule the some async operations
 * TODO(MBkkt) add description
 */
template <typename V = void, typename E = StopError>
class [[nodiscard]] Task final {
 public:
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>, "Task cannot be instantiated with same V and E, because it's ambiguous");

  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  Task(Task&& other) noexcept = default;
  Task& operator=(Task&& other) noexcept = default;

  Task() noexcept = default;
  ~Task() noexcept;

  [[nodiscard]] bool Valid() const& noexcept;

  /**
   * TODO(MBkkt) think about force On/Detach/ToFuture:
   *  It's able to set passed executor to previous nullptr/all/head/etc or replace
   */
  Task<V, E> On(IExecutor& e) && noexcept;
  Task<V, E> On(std::nullptr_t) && noexcept;

  template <typename Func>
  /*Task*/ auto Then(IExecutor& e, Func&& f) &&;
  template <typename Func>
  /*Task*/ auto ThenInline(Func&& f) &&;
  template <typename Func>
  /*Task*/ auto Then(Func&& f) &&;

  void Cancel() &&;

  void Detach(IExecutor& e = MakeInline()) && noexcept;

  Future<V, E> ToFuture() && noexcept;
  FutureOn<V, E> ToFuture(IExecutor& e) && noexcept;

  Result<V, E> Get(IExecutor& e = MakeInline()) && noexcept {  // Stub implementation
    // TODO(MBkkt) make it better: we can remove concurrent atomic changes from here
    return std::move(*this).ToFuture(e).Get();
  }

  /**
   * Method that get internal Core state
   *
   * \return internal Core state ptr
   */
  [[nodiscard]] detail::ResultCorePtr<V, E>& GetCore() noexcept;
  Task(detail::ResultCorePtr<V, E> core) noexcept;

 private:
  detail::ResultCorePtr<V, E> _core;
};

extern template class Task<void, StopError>;

namespace detail {

template <typename E, typename Func>
auto Schedule(IExecutor* e, Func&& f) {
  auto* core = detail::MakeCore<detail::CoreType::Run, void, E>(std::forward<Func>(f));
  core->_executor = e;
  using ResultCoreT = typename std::remove_reference_t<decltype(*core)>::Base;
  return Task{IntrusivePtr<ResultCoreT>{NoRefTag{}, core}};
}

template <bool Force = false>
void Run(detail::BaseCore* head, IExecutor& e) noexcept;

extern template void Run<false>(detail::BaseCore* head, IExecutor& e) noexcept;
extern template void Run<true>(detail::BaseCore* head, IExecutor& e) noexcept;

}  // namespace detail

template <typename V, typename E>
Task<V, E>::~Task() noexcept {
  if (Valid()) {
    std::move(*this).Cancel();
  }
}

template <typename V, typename E>
bool Task<V, E>::Valid() const& noexcept {
  return _core != nullptr;
}

template <typename V, typename E>
Task<V, E> Task<V, E>::On(IExecutor& e) && noexcept {
  _core->_executor = &e;
  return {std::move(_core)};
}

template <typename V, typename E>
Task<V, E> Task<V, E>::On(std::nullptr_t) && noexcept {
  _core->_executor = nullptr;
  return {std::move(this->_core)};
}

template <typename V, typename E>
template <typename Func>
auto Task<V, E>::Then(IExecutor& e, Func&& f) && {
  _core->_executor = &e;
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::Lazy>(_core, std::forward<Func>(f));
}

template <typename V, typename E>
template <typename Func>
auto Task<V, E>::ThenInline(Func&& f) && {
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::LazyInline>(_core, std::forward<Func>(f));
}

template <typename V, typename E>
template <typename Func>
auto Task<V, E>::Then(Func&& f) && {
  return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::Lazy>(_core, std::forward<Func>(f));
}

template <typename V, typename E>
void Task<V, E>::Cancel() && {
  _core->StoreCallback(static_cast<IRef*>(&MakeInline(StopTag{})), detail::InlineCore::kWaitDrop);
  detail::Run<true>(_core.Release(), MakeInline(StopTag{}));
}

template <typename V, typename E>
void Task<V, E>::Detach(IExecutor& e) && noexcept {
  _core->StoreCallback(static_cast<IRef*>(&MakeInline()), detail::InlineCore::kWaitDrop);
  detail::Run(_core.Release(), e);
}

template <typename V, typename E>
Future<V, E> Task<V, E>::ToFuture() && noexcept {
  detail::Run(_core.Get(), MakeInline());
  _core->_executor = nullptr;
  return {std::move(_core)};
}

template <typename V, typename E>
FutureOn<V, E> Task<V, E>::ToFuture(IExecutor& e) && noexcept {
  YACLIB_WARN(e.Tag() == IExecutor::Type::Inline, "better way is use ToFuture() instead of ToFuture(MakeInline())");
  detail::Run(_core.Get(), e);
  _core->_executor = &e;
  return {std::move(_core)};
}

template <typename V, typename E>
detail::ResultCorePtr<V, E>& Task<V, E>::GetCore() noexcept {
  return _core;
}

template <typename V, typename E>
Task<V, E>::Task(detail::ResultCorePtr<V, E> core) noexcept : _core{std::move(core)} {
}

/**
 * TODO(MBkkt) add description
 */
template <typename E = StopError, typename Func>
/*Task*/ auto Schedule(IExecutor& e, Func&& f) {
  auto task = detail::Schedule<E>(&e, std::forward<Func>(f));
  e.IncRef();
  task.GetCore()->_caller = &e;
  return task;
}

/**
 * TODO(MBkkt) add description
 */
template <typename E = StopError, typename Func>
/*Task*/ auto Schedule(Func&& f) {
  auto task = detail::Schedule<E>(nullptr, std::forward<Func>(f));
  task.GetCore()->_caller = nullptr;
  return task;
}

/**
 * TODO(MBkkt) add description
 */
template <typename V = Unit, typename E = StopError, typename... Args>
/*Task*/ auto MakeTask(Args&&... args) {
  // TODO(MBkkt) optimize sizeof: now we store Result twice
  if constexpr (sizeof...(Args) == 0) {
    return Schedule<E>(MakeInline(), [] {
    });
  } else if constexpr (sizeof...(Args) == 1) {
    using T = std::conditional_t<std::is_same_v<V, Unit>, head_t<Args...>, V>;
    static_assert(!std::is_same_v<T, Unit>);
    return Schedule<E>(MakeInline(), [result = Result<T, E>{std::forward<Args>(args)...}]() mutable {
      return std::move(result);
    });
  } else {
    static_assert(!std::is_same_v<V, Unit>);
    return Schedule<E>(MakeInline(), [result = Result<V, E>{std::forward<Args>(args)...}]() mutable {
      return std::move(result);
    });
  }
}

}  // namespace yaclib
