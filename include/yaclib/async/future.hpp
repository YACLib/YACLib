#pragma once

#include <yaclib/algo/detail/core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/async/wait.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

/**
 * Provides a mechanism to access the result of async operations
 *
 * Future and \ref Promise are like a Single Producer/Single Consumer one-shot one-element channel.
 * Use the \ref Promise to fulfill the \ref Future.
 */
template <typename V, typename E>
class FutureBase {
 public:
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>, "Future cannot be instantiated with same V and E, because it's ambiguous");

  FutureBase(const FutureBase&) = delete;
  FutureBase& operator=(const FutureBase&) = delete;

  FutureBase(FutureBase&& other) noexcept = default;
  FutureBase& operator=(FutureBase&& other) noexcept = default;

  /**
   * The default constructor creates not a \ref Valid Future
   *
   * Needed only for usability, e.g. instead of std::optional<Future<T>> in containers.
   */
  FutureBase() noexcept = default;

  /**
   * If Future is \ref Valid then call \ref Stop
   */
  ~FutureBase() noexcept {
    if (Valid()) {
      std::move(*this).Detach();
    }
  }

  /**
   * Check if this \ref Future has \ref Promise
   *
   * \return false if this \ref Future is default-constructed or moved to, otherwise true
   */
  [[nodiscard]] bool Valid() const& noexcept {
    return _core != nullptr;
  }

  /**
   * Check that \ref Result that corresponds to this \ref Future is computed
   *
   * \return false if the \ref Result of this \ref Future is not computed yet, otherwise true
   */
  [[nodiscard]] bool Ready() const& noexcept {
    YACLIB_ASSERT(Valid());
    return !_core->Empty();
  }

  void Get() & = delete;
  void Get() const&& = delete;
  void Touch() & = delete;
  void Touch() const&& = delete;

  /**
   * Return copy of \ref Result from \ref Future
   *
   * If \ref Ready is false return an empty \ref Result. This method is thread-safe and can be called multiple times.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \return \ref Result stored in the shared state
   */
  [[nodiscard]] const Result<V, E>* Get() const& noexcept {
    if (Ready()) {  // TODO(MBkkt) Maybe we want likely
      return &_core->Get();
    }
    return nullptr;
  }

  /**
   * Wait until \def Ready is true and move \ref Result from Future
   *
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \return The \ref Result that Future received
   */
  [[nodiscard]] Result<V, E> Get() && noexcept {
    Wait(*this);
    auto core = std::exchange(_core, nullptr);
    return std::move(core->Get());
  }

  /**
   * Assume \def Ready is true and return copy reference to \ref Result from Future
   *
   * Assume Ready is true. This method is NOT thread-safe and can be called multiple
   * \note The behavior is undefined if \ref Valid or Ready is false before the call to this function.
   * \return The \ref Result stored in the shared state
   */
  [[nodiscard]] const Result<V, E>& Touch() const& noexcept {
    YACLIB_ASSERT(Ready());
    return _core->Get();
  }

  /**
   * Assume \def Ready is true and move \ref Result from Future
   *
   * \note The behavior is undefined if \ref Valid or Ready is false before the call to this function.
   * \return The \ref Result that Future received
   */
  [[nodiscard]] Result<V, E> Touch() && noexcept {
    YACLIB_ASSERT(Ready());
    auto core = std::exchange(_core, nullptr);
    return std::move(core->Get());
  }

  /**
   * Attach the continuation func to *this
   *
   * The func will be executed on the specified executor.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param e Executor which will \ref Execute the continuation
   * \param f A continuation to be attached
   * \return New \ref FutureOn object associated with the func result
   */
  template <typename Func>
  [[nodiscard]] /*FutureOn*/ auto Then(IExecutor& e, Func&& f) && {
    YACLIB_WARN(e.Tag() == IExecutor::Type::Inline,
                "better way is use ThenInline(...) instead of Then(MakeInline(), ...)");
    return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::On>(_core, &e, std::forward<Func>(f));
  }

  /**
   * Disable calling \ref Stop in destructor
   */
  void Detach() && noexcept {
    auto* core = _core.Release();
    // TODO(MBkkt) if use SetCallback it will single virtual call instead of two
    core->CallInline(detail::MakeDrop());
  }

  /**
   * Attach the final continuation func to *this and \ref Detach *this
   *
   * The func will be executed on \ref Inline executor.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param f A continuation to be attached
   */
  template <typename Func>
  void DetachInline(Func&& f) && {
    detail::SetCallback<detail::CoreType::Detach, detail::CallbackType::Inline>(_core, nullptr, std::forward<Func>(f));
  }

  /**
   * Attach the final continuation func to *this and \ref Detach *this
   *
   * The func will be executed on the specified executor.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param e Executor which will \ref Execute the continuation
   * \param f A continuation to be attached
   */
  template <typename Func>
  void Detach(IExecutor& e, Func&& f) && {
    YACLIB_WARN(e.Tag() == IExecutor::Type::Inline,
                "better way is use DetachInline(...) instead of Detach(MakeInline(), ...)");
    detail::SetCallback<detail::CoreType::Detach, detail::CallbackType::On>(_core, &e, std::forward<Func>(f));
  }

  /**
   * Method that get internal Core state
   *
   * \return internal Core state ptr
   */
  [[nodiscard]] detail::ResultCorePtr<V, E>& GetCore() noexcept {
    return _core;
  }

 protected:
  explicit FutureBase(detail::ResultCorePtr<V, E> core) noexcept : _core{std::move(core)} {
  }

  detail::ResultCorePtr<V, E> _core;
};

extern template class FutureBase<void, StopError>;

/**
 * Provides a mechanism to access the result of async operations
 *
 * Future and \ref Promise are like a Single Producer/Single Consumer one-shot one-element channel.
 * Use the \ref Promise to fulfill the \ref Future.
 */
template <typename V, typename E>
class Future final : public FutureBase<V, E> {
  using Base = FutureBase<V, E>;

 public:
  using Base::Base;

  Future(detail::ResultCorePtr<V, E> core) noexcept : Base{std::move(core)} {
  }

  /**
   * Attach the continuation func to *this
   *
   * The func will be executed on \ref Inline executor.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param f A continuation to be attached
   * \return New \ref Future object associated with the func result
   */
  template <typename Func>
  [[nodiscard]] /*Future*/ auto ThenInline(Func&& f) && {
    return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::Inline>(this->_core, nullptr,
                                                                                     std::forward<Func>(f));
  }
};

extern template class Future<>;

/**
 * Provides a mechanism to access the result of async operations
 *
 * Future and \ref Promise are like a Single Producer/Single Consumer one-shot one-element channel.
 * Use the \ref Promise to fulfill the \ref Future.
 */
template <typename V, typename E>
class FutureOn final : public FutureBase<V, E> {
  using Base = FutureBase<V, E>;

 public:
  using Base::Base;
  using Base::Detach;
  using Base::Then;

  FutureOn(detail::ResultCorePtr<V, E> core) noexcept : Base{std::move(core)} {
  }

  /**
   * Specify executor for continuation.
   * Make FutureOn -- Future with executor
   */
  [[nodiscard]] Future<V, E> On(std::nullptr_t) && noexcept {
    return {std::move(this->_core)};
  }

  /**
   * Attach the continuation func to *this
   *
   * The func will be executed on \ref Inline executor.
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param f A continuation to be attached
   * \return New \ref FutureOn object associated with the func result
   */
  template <typename Func>
  [[nodiscard]] /*FutureOn*/ auto ThenInline(Func&& f) && {
    return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::InlineOn>(this->_core, nullptr,
                                                                                       std::forward<Func>(f));
  }

  /**
   * Attach the continuation func to *this
   *
   * \note The behavior is undefined if \ref Valid is false before the call to this function.
   * \param f A continuation to be attached
   * \return New \ref FutureOn object associated with the func result
   */
  template <typename Func>
  [[nodiscard]] /*FutureOn*/ auto Then(Func&& f) && {
    return detail::SetCallback<detail::CoreType::Then, detail::CallbackType::On>(this->_core, nullptr,
                                                                                 std::forward<Func>(f));
  }

  /**
   * Attach the final continuation func to *this and \ref Detach *this
   *
   * \note Func must return void type.
   * \param f A continuation to be attached
   */
  template <typename Func>
  void Detach(Func&& f) && {
    detail::SetCallback<detail::CoreType::Detach, detail::CallbackType::On>(this->_core, nullptr,
                                                                            std::forward<Func>(f));
  }
};

extern template class FutureOn<>;

}  // namespace yaclib
