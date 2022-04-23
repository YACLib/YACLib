#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/job.hpp>
#include <yaclib/util/detail/nope_counter.hpp>

namespace yaclib {
namespace detail {

class Inline : public IExecutor {
 private:
  [[nodiscard]] Type Tag() const final {
    return Type::Inline;
  }

  void Submit(Job& task) noexcept final {
    task.Call();
  }
};

static NopeCounter<Inline> sInline;  // TODO(MBkkt) Make file with depended globals

}  // namespace detail

IExecutor& MakeInline() noexcept {
  return detail::sInline;
}

}  // namespace yaclib
