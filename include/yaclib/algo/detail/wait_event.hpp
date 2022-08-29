#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/util/detail/set_deleter.hpp>

namespace yaclib::detail {

template <typename Event, template <typename...> typename Counter>
class WaitEvent final : public InlineCore, public Counter<Event, SetDeleter> {
 public:
  using Counter<Event, SetDeleter>::Counter;

 private:
  void Call() noexcept final {
    this->Sub(1);
  }
};

}  // namespace yaclib::detail
