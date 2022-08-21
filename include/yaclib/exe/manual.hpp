#pragma once

#include <util/intrusive_list.hpp>

#include <yaclib/exe/executor.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib {
/**
 * TODO(mkornaukhov03) Doxygen
 */
class ManualExecutor : public IExecutor {
 private:
  yaclib::detail::List _tasks;

 public:
  [[nodiscard]] Type Tag() const noexcept final;

  void Submit(yaclib::Job& f) noexcept final;

  std::size_t Drain();
};

IntrusivePtr<ManualExecutor> MakeManual();

}  // namespace yaclib
