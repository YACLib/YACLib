#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/job.hpp>
#include <yaclib/util/detail/nope_counter.hpp>

namespace yaclib {
namespace detail {

class Inline final : public NopeCounter<IExecutor> {
 private:
  [[nodiscard]] Type Tag() const final {
    return Type::Inline;
  }

  void Submit(Job& task) noexcept final {
    task.Call();
  }
};

// TODO(MBkkt) Make file with depended globals
static Inline sInline;

}  // namespace detail

IExecutor& MakeInline() noexcept {
  return detail::sInline;
}

}  // namespace yaclib
