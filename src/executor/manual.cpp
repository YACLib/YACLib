#include <util/intrusive_list.hpp>

#include <yaclib/executor/manual.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib {
IExecutor::Type ManualExecutor::Tag() const {
  return yaclib::IExecutor::Type::Custom;
}

void ManualExecutor::Submit(yaclib::Job& f) noexcept {
  _tasks.PushBack(f);
}

std::size_t ManualExecutor::Drain() {
  std::size_t response = 0;
  while (!_tasks.Empty()) {
    response++;
    auto& task = _tasks.PopFront();
    static_cast<yaclib::Job&>(task).Call();
  }
  return response;
}

ManualExecutor::~ManualExecutor() {
}

IntrusivePtr<ManualExecutor> MakeManual() {
  return MakeIntrusive<ManualExecutor>();
}

}  // namespace yaclib
