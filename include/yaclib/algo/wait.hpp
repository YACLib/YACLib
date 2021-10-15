#pragma once

#include <yaclib/algo/detail/wait.hpp>

namespace yaclib {

/**
 * Wait until \ref Ready becomes true
 *
 * \param futures one or more futures to wait
 */
template <typename... Fs>
void Wait(Fs&&... futures) {
  detail::Wait<detail::WaitPolicy::Endless>(/* stub value */ false, static_cast<detail::BaseCore&>(*futures._core)...);
}

}  // namespace yaclib
