#pragma once

#include <util/intrusive_list.hpp>

#include <yaclib/executor/executor.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>


namespace yaclib {
/**
 * TODO(mkornaukhov03) Doxygen
 */
class ManualExecutor : public yaclib::IExecutor {
 private:
  yaclib::detail::List _tasks;

 public:
  [[nodiscard]] Type Tag() const final;

  void Submit(yaclib::Job& f) noexcept final;

  void Drain();

  ~ManualExecutor() override;
};


IntrusivePtr<ManualExecutor> MakeManual();

}  // namespace yaclib
