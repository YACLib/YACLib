#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

/**
 * Provides a mechanism to schedule the some async operations
 */
template <typename V = void, typename E = StopError>
class [[nodiscard]] Task {
 public:
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>, "Task cannot be instantiated with same V and E, because it's ambiguous");

  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  Task(Task&& other) noexcept = default;
  Task& operator=(Task&& other) noexcept = default;

  /**
   * The default constructor creates not a \ref Valid Task
   *
   * Needed only for usability, e.g. instead of std::optional<Task<T>> in containers.
   */
  Task() noexcept = default;

  /**
   * If Task is \ref Valid then call \ref Stop
   */
  ~Task() noexcept;

  /**
   * Attach the continuation func to *this
   *
   * The func will be executed on the specified executor.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param e Executor which will \ref Execute the continuation
   * \param f A continuation to be attached
   * \return New \ref TaskOn object associated with the func result
   */
  template <typename Func>
  /*Task*/ auto Then(IExecutor& e, Func&& f) && {
  }

  /**
   * Attach the continuation func to *this
   *
   * The func will be executed on \ref Inline executor.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param f A continuation to be attached
   * \return New \ref TaskOn object associated with the func result
   */
  template <typename Func>
  /*Task*/ auto ThenInline(Func&& f) && {
  }

  FutureOn<V, E> Run() {
  }
  /**
   * Method that get internal Core state
   *
   * \return internal Core state ptr
   */
  [[nodiscard]] detail::ResultCorePtr<V, E>& GetCore() noexcept;

  explicit Task(detail::ResultCorePtr<V, E> core) noexcept;

 private:
  detail::ResultCorePtr<V, E> _core;
};

extern template class Task<void, StopError>;

/**
 * Execute Callable func on executor
 *
 * \param e executor to be used to execute f and saved as callback executor for return \ref Future
 * \param f func to execute
 * \return \ref Task corresponding f return value
 */
template <typename E = StopError, typename Func>
auto Lazy(IExecutor& e, Func&& f) {
  auto* core = detail::MakeCore<detail::CoreType::Run, void, E>(std::forward<Func>(f));
  core->SetExecutor(&e);
  using ResultCoreT = typename std::remove_reference_t<decltype(*core)>::Base;
  return Task{IntrusivePtr<ResultCoreT>{NoRefTag{}, core}};
}

}  // namespace yaclib
