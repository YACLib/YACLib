#include <yaclib/exe/manual.hpp>
#include <yaclib/util/helper.hpp>

namespace yaclib {

IExecutor::Type ManualExecutor::Tag() const noexcept {
  return Type::Manual;
}

bool ManualExecutor::Alive() const noexcept {
  return true;
}

void ManualExecutor::Submit(Job& f) noexcept {
  _tasks.PushBack(f);
}

std::size_t ManualExecutor::Drain() noexcept {
  std::size_t done = 0;
  while (!_tasks.Empty()) {
    ++done;
    auto& task = _tasks.PopFront();
    static_cast<Job&>(task).Call();
  }
  return done;
}

IExecutorPtr MakeManual() {
  return MakeShared<ManualExecutor>(1);
}

}  // namespace yaclib
