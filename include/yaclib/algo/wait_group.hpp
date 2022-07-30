#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/default_event.hpp>
#include <yaclib/util/detail/set_all_deleter.hpp>
#include <yaclib/util/type_traits.hpp>

#include <cstddef>

namespace yaclib {

/**
 * An object that allows you to Add some amount of async operations and then Wait for it to be Done
 *
 * \tparam InitCount of Add that will be stored in default ctor or Reset
 * \tparam Event special object needed to wait
 */
template <std::size_t InitCount = 1, typename Event = detail::DefaultEvent>
class WaitGroup final {
 public:
  explicit WaitGroup(std::size_t count = InitCount) : _event{count} {
  }

  /**
   * Add some amount of async operations
   *
   * Can be called parallel with Add, Done,
   * and with Wait, but only if you call it when some Add not Done yet
   *
   * \param count of async operations
   */
  void Add(std::size_t count = 1) noexcept;

  /**
   * Done some Add-ed async operations
   *
   * \param count of async operations
   */
  void Done(std::size_t count = 1) noexcept;

  /**
   * Add \ref Future to WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add
   * \param futures to wait
   */
  template <bool NeedAdd = true, typename... V, typename... E>
  YACLIB_INLINE void Add(FutureBase<V, E>&... futures) noexcept {
    AddCore<NeedAdd>(static_cast<detail::BaseCore&>(*futures.GetCore())...);
  }

  /**
   * Add \ref Future to WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add
   * \param begin iterator to futures to Add
   * \param end iterator to futures to Add
   */
  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE std::enable_if_t<!is_future_base_v<It>, void> Add(It begin, It end) noexcept {
    AddIterator<NeedAdd>(begin, end - begin);
  }

  /**
   * Add \ref Future to WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add
   * \param begin iterator to futures to Add
   * \param count count of futures to Add
   */
  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE void Add(It begin, std::size_t count) noexcept {
    AddIterator<NeedAdd>(begin, count);
  }

  /**
   * Move \ref Future to WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add
   * \param futures to wait
   */
  template <bool NeedAdd = true, typename... V, typename... E>
  YACLIB_INLINE void Move(FutureBase<V, E>&&... futures) noexcept {
    MoveCore<NeedAdd>(static_cast<detail::BaseCore&>(*futures.GetCore().Release())...);
  }

  /**
   * Move \ref Future to WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add
   * \param begin iterator to futures to Add
   * \param end iterator to futures to Add
   */
  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE std::enable_if_t<!is_future_base_v<It>, void> Move(It begin, It end) noexcept {
    MoveIterator<NeedAdd>(begin, end - begin);
  }

  /**
   * Move \ref Future to WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add
   * \param begin iterator to futures to Add
   * \param count count of futures to Add
   */
  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE void Move(It begin, std::size_t count) noexcept {
    MoveIterator<NeedAdd>(begin, count);
  }

  /**
   * Waiting for everything Add-ed to Done
   * For all Wait you should make Add
   *
   * \tparam NeedDone if true make implicit Done, if false you should make explicit Done
   */
  template <bool NeedDone = true>
  void Wait() noexcept;

  /**
   * Reinitializes WaitGroup, semantically the same as `*this = {};`
   *
   * If you don't explicitly call this method,
   * then after the first one, Wait will always return immediately.
   *
   * \note Not thread-safe
   */
  void Reset() noexcept;

 private:
  template <bool NeedAdd, typename... Cores>
  void AddCore(Cores&... cores) noexcept;

  template <bool NeedAdd, typename Iterator>
  void AddIterator(Iterator it, std::size_t count) noexcept;

  template <bool NeedAdd, typename Range>
  void AddRange(const Range& range, std::size_t count) noexcept;

  template <bool NeedAdd, typename... Cores>
  void MoveCore(Cores&... cores) noexcept;

  template <bool NeedAdd, typename Iterator>
  void MoveIterator(Iterator it, std::size_t count) noexcept;

  template <bool NeedAdd, typename Range>
  void MoveRange(const Range& range, std::size_t count) noexcept;

  using EventCore = detail::AtomicCounter<Event, detail::SetAllDeleter>;
  EventCore _event{InitCount};
};

extern template class WaitGroup<1, detail::DefaultEvent>;

}  // namespace yaclib

#include <yaclib/algo/detail/wait_group_impl.hpp>
