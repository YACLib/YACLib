#pragma once

#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/util/detail/intrusive_list.hpp>

#include <cstddef>

namespace yaclib {

/**
 * TODO(mkornaukhov03) Doxygen
 */
class ManualExecutor : public IExecutor {
 public:
  [[nodiscard]] Type Tag() const noexcept final;

  [[nodiscard]] bool Alive() const noexcept final;

  void Submit(Job& f) noexcept final;

  [[nodiscard]] std::size_t Drain() noexcept;

 private:
  detail::List _tasks;
};

IExecutorPtr MakeManual();

}  // namespace yaclib
