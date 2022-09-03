#include <util/intrusive_list.hpp>

#include <yaclib/exe/manual.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib {

IExecutor::Type ManualExecutor::Tag() const noexcept {
  return yaclib::IExecutor::Type::Custom;
}

bool ManualExecutor::Alive() const noexcept {
  return true;
}

void ManualExecutor::Submit(yaclib::Job& f) noexcept {
  _tasks.PushBack(f);
}

std::size_t ManualExecutor::Drain() {
  std::size_t done = 0;
  while (!_tasks.Empty()) {
    ++done;
    auto& task = _tasks.PopFront();
    static_cast<yaclib::Job&>(task).Call();
  }
  return done;
}

IntrusivePtr<ManualExecutor> MakeManual() {
  return MakeShared<ManualExecutor>(1);
}

}  // namespace yaclib
