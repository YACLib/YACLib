#include <yaclib/executor/executor.hpp>

namespace yaclib::executor {

class Inline final : public IExecutor {
 private:
  Type Tag() const final {
    return Type::Inline;
  }

  bool Execute(ITask& task) final {
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

}  // namespace yaclib::executor
