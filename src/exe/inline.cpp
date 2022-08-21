#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/util/detail/nope_counter.hpp>

namespace yaclib {
namespace detail {

template <bool Stop>
class Inline final : public NopeCounter<IExecutor> {
 private:
  [[nodiscard]] Type Tag() const noexcept final {
    return Type::Inline;
  }

  void Submit(Job& task) noexcept final {
    if constexpr (Stop) {
      task.Drop();
    } else {
      task.Call();
    }
  }
};

// TODO(MBkkt) Make file with depended globals
static Inline<false> sAliveInline;
static Inline<true> sStopInline;

}  // namespace detail

IExecutor& MakeInline() noexcept {
  return detail::sAliveInline;
}

IExecutor& MakeInline(StopTag) noexcept {
  return detail::sStopInline;
}

}  // namespace yaclib
