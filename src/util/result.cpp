#include <yaclib/util/result.hpp>

namespace yaclib {

template class Result<>;

const std::exception_ptr& StopPtr() noexcept {
  // yaclib objects must not have static storage duration:
  // e.g. a Promise destroyed during static destruction would cancel after this is destroyed
  static const std::exception_ptr stop = std::make_exception_ptr(StopException{});
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
    return false;
  }
}

}  // namespace yaclib
