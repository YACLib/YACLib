#pragma once

#include <iterator>

#if YACLIB_CXX_STANDARD < 20 ||                                                                                        \
  (defined(__clang__) && __clang_major__ <= 12)  // std::counted_iterator not implemented in stl

namespace yaclib::detail {

struct DefaultSentinel {};

template <typename It>
class counted_iterator {
 public:
  using iterator_category = typename std::iterator_traits<It>::iterator_category;
  using value_type = typename std::iterator_traits<It>::value_type;
  using difference_type = typename std::iterator_traits<It>::difference_type;
  using pointer = typename std::iterator_traits<It>::pointer;
  using reference = typename std::iterator_traits<It>::reference;

  counted_iterator() : _it{}, _n{0} {
  }

  counted_iterator(It it, difference_type n) : _it{it}, _n{n} {
  }

  reference operator*() const {
    return *_it;
  }

  pointer operator->() const {
    return _it.operator->();
  }

  counted_iterator& operator++() {
    ++_it;
    --_n;
    return *this;
  }

  counted_iterator operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  bool operator==(const counted_iterator& rhs) const {
    return _it == rhs._it;
  }

  bool operator!=(const counted_iterator& rhs) const {
    return !(*this == rhs);
  }

 private:
  It _it;
  difference_type _n;

  friend bool operator==(const counted_iterator<It>& lhs, [[maybe_unused]] DefaultSentinel rhs) {
    return lhs._n == 0;
  }

  friend bool operator==(DefaultSentinel lhs, const counted_iterator<It>& rhs) {
    return rhs == lhs;
  }

  friend bool operator!=(const counted_iterator<It>& lhs, DefaultSentinel rhs) {
    return !(lhs == rhs);
  }

  friend bool operator!=(DefaultSentinel lhs, const counted_iterator<It>& rhs) {
    return !(lhs == rhs);
  }

  friend difference_type operator-(const counted_iterator<It> lhs, [[maybe_unused]] DefaultSentinel rhs) {
    return -lhs._n;
  }

  friend difference_type operator-(DefaultSentinel lhs, const counted_iterator<It> rhs) {
    return -(rhs - lhs);
  }
};

}  // namespace yaclib::detail

namespace yaclib_std {

using default_sentinel_t = yaclib::detail::DefaultSentinel;

inline constexpr default_sentinel_t default_sentinel{};

using yaclib::detail::counted_iterator;

}  // namespace yaclib_std

#else

namespace yaclib_std {

using std::default_sentinel_t;

inline constexpr default_sentinel_t default_sentinel{};

using std::counted_iterator;

}  // namespace yaclib_std

#endif