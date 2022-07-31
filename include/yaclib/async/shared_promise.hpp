#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename V, typename E = StopError>
class SharedPromise final {
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>,
                "SharedPromise cannot be instantiated with same V and E, because it's ambiguous");

 public:
  SharedPromise(const SharedPromise& other) = delete;
  SharedPromise& operator=(const SharedPromise& other) = delete;

  SharedPromise(SharedPromise&& other) noexcept = default;
  SharedPromise& operator=(SharedPromise&& other) noexcept = default;

  /**
   * The default constructor creates not a \ref Valid SharedPromise
   *
   * Needed only for usability, e.g. instead of std::optional<SharedPromise<T>> in containers.
   */
  SharedPromise() : _head{nullptr}, _result{}, _is_set{false} {
  }

  /**
   * If SharedPromise is \ref Valid then set \ref StopTag
   */
  ~SharedPromise();

  /**
   * Check if this \ref SharedPromise has \ref Future
   *
   * \return false if this \ref SharedPromise is default-constructed or moved to, otherwise true
   */
  [[nodiscard]] bool Valid() const& noexcept;

  /**
   * Set \ref SharedPromise result
   *
   * \tparam Type \ref Result<T> should be constructable from this type
   * \param value value
   */

  template <typename Type>
  void Set(Type&& value);

  /**
   * Set \ref SharedPromise<void> result
   */
  void Set();

  Future<V, E> GetFuture();

  /**
   * Part of unsafe but internal API
   * explicit SharedPromise(detail::ResultCorePtr<V, E> core) noexcept;
   * [[nodiscard]] detail::ResultCorePtr<V, E>& GetCore() noexcept;
   **/

 private:
  using ResultCoreType = detail::ResultCore<V, E>;

  yaclib_std::atomic<ResultCoreType*> _head;
  Result<V, E> _result;
  yaclib_std::atomic_bool _is_set;
};

extern template class SharedPromise<void, StopError>;

}  // namespace yaclib

#include <yaclib/async/detail/shared_promise_impl.hpp>
