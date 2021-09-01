#include <yaclib/executor/executor.hpp>

namespace yaclib::executor {

class Inline final : public IExecutor {
 private:
  bool Execute(ITask& task) override {
    task.Call();
    return true;
  }

  void IncRef() noexcept final {
  }
  void DecRef() noexcept final {
  }
};

IExecutorPtr MakeInlineExecutor() noexcept {
  static Inline sInline;
  return &sInline;
}

}  // namespace yaclib::executor
