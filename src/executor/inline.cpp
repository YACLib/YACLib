#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/job.hpp>

namespace yaclib {

class Inline final : public IExecutor {
 private:
  [[nodiscard]] Type Tag() const final {
    return Type::Inline;
  }

  void Submit(Job& task) noexcept final {
    task.Call();
  }

  void IncRef() noexcept final {
  }
  void DecRef() noexcept final {
  }
};

static Inline sInline;  // TODO(MBkkt) correct?

IExecutor& MakeInline() noexcept {
  return sInline;
}

}  // namespace yaclib
