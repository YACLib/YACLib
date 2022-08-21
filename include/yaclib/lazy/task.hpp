#pragma once

#include <yaclib/algo/detail/result_core.hpp>
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

  Result<V, E> Get(IExecutor& e = MakeInline()) && noexcept;

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

}  // namespace yaclib

#include <yaclib/lazy/detail/task_impl.hpp>
