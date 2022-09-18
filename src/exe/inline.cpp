#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/exe/job.hpp>

namespace yaclib {
namespace {

template <bool Stopped>
class Inline final : public IExecutor {
 private:
  [[nodiscard]] Type Tag() const noexcept final {
    return Type::Inline;
  }

  [[nodiscard]] bool Alive() const noexcept final {
    return !Stopped;
  }

  void Submit(Job& task) noexcept final {
    if constexpr (Stopped) {
      task.Drop();
    } else {
      task.Call();
    }
  }
};

// TODO(MBkkt) Make file with depended globals
static Inline<false> sCallInline;
static Inline<true> sDropInline;

}  // namespace

IExecutor& MakeInline() noexcept {
  return sCallInline;
}

IExecutor& MakeInline(StopTag) noexcept {
  return sDropInline;
}

}  // namespace yaclib
