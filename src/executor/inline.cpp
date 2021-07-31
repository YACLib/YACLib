#include <yaclib/executor/executor.hpp>

#include <memory>

namespace yaclib::executor {

class Inline final : public IExecutor {
 private:
  void Execute(ITask& task) override {
    task.Call();
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
