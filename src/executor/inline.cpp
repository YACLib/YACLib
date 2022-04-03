#include <yaclib/config.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/task.hpp>

namespace yaclib {

class Inline final : public IExecutor {
 private:
  [[nodiscard]] Type Tag() const final {
    return Type::Inline;
  }

  void Submit(ITask& task) noexcept final {
    task.Call();
  }

  void IncRef() noexcept final {
  }
  void DecRef() noexcept final {
  }
};

static Inline sInline;  // TODO(MBkkt) correct?

IExecutorPtr MakeInline() noexcept {
  return &sInline;
}

}  // namespace yaclib
