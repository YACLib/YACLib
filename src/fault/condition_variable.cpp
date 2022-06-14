#include <yaclib/fault/detail/condition_variable.hpp>

namespace yaclib::detail {

constexpr std::cv_status CVStatusFrom(WaitStatus status) {
  if (status == WaitStatus::Ready) {
    return std::cv_status::no_timeout;
  }
  return std::cv_status::timeout;
}

constexpr std::cv_status CVStatusFrom(std::cv_status status) {
  return status;
}

}  // namespace yaclib::detail
