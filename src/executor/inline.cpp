#include <yaclib/executor/inline.hpp>

namespace yaclib {

class Inline final : public IExecutor {
 private:
  [[nodiscard]] Type Tag() const final {
    return Type::Inline;
  }

  bool Execute(ITask& task) noexcept final {
    task.Call();
    return true;
  }

  void IncRef() noexcept final {
  }
  void DecRef() noexcept final {
  }
};

IExecutorPtr MakeInline() noexcept {
  static Inline sInline;
  return &sInline;
}

}  // namespace yaclib
