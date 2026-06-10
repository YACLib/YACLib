#include <yaclib/util/result.hpp>

namespace yaclib {

template class Result<>;

const std::exception_ptr& StopPtr() noexcept {
  // Intentionally immortal (never destroyed): cancellation runs in noexcept teardown paths
  // that may execute after static destruction, e.g. destructors of user static objects
  static const std::exception_ptr& stop = *new std::exception_ptr{std::make_exception_ptr(StopException{})};
  return stop;
}

bool IsStop(const std::exception_ptr& error) noexcept {
  if (error == nullptr) {
    return false;
  }
  if (error == StopPtr()) {
    return true;
  }
  try {
    std::rethrow_exception(error);
  } catch (const StopException&) {
    return true;
  } catch (...) {
  }
  return false;
}

}  // namespace yaclib
