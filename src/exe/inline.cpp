#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/exe/job.hpp>

namespace yaclib {
namespace {

template <bool Stop>
class Inline final : public IExecutor {
 private:
  [[nodiscard]] Type Tag() const noexcept final {
    return Type::Inline;
  }

  [[nodiscard]] bool Alive() const noexcept final {
    return true;
  }

  void Submit(Job& task) noexcept final {
    if constexpr (Stop) {
      task.Drop();
    } else {
      task.Call();
    }
  }
};

// TODO(MBkkt) Make file with depended globals
static Inline<false> sAliveInline;
static Inline<true> sStopInline;

}  // namespace

IExecutor& MakeInline() noexcept {
  return sAliveInline;
}

IExecutor& MakeInline(StopTag) noexcept {
  return sStopInline;
}

}  // namespace yaclib
