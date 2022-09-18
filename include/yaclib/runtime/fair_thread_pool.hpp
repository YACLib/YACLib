#pragma once

#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/util/detail/intrusive_list.hpp>

#include <cstdint>
#include <vector>
#include <yaclib_std/condition_variable>
#include <yaclib_std/mutex>
#include <yaclib_std/thread>

namespace yaclib {

/**
 * TODO(kononovk) Doxygen docs
 */
class FairThreadPool : public IExecutor {
 public:
  explicit FairThreadPool(std::uint64_t threads = yaclib_std::thread::hardware_concurrency());

  ~FairThreadPool() noexcept override;

  Type Tag() const noexcept final;

  bool Alive() const noexcept final;

  void Submit(Job& task) noexcept final;

  void SoftStop() noexcept;

  void Stop() noexcept;

  void HardStop() noexcept;

  /**
   * TODO(kononovk) Rename to Join
   */
  void Wait() noexcept;

 private:
  void Loop() noexcept;

  [[nodiscard]] bool WasStop() const noexcept;
  [[nodiscard]] bool WantStop() const noexcept;
  [[nodiscard]] bool NoJobs() const noexcept;

  void Stop(std::unique_lock<yaclib_std::mutex>&& lock) noexcept;

  std::vector<yaclib_std::thread> _workers;
  mutable yaclib_std::mutex _m;
  yaclib_std::condition_variable _idle;
  detail::List _jobs;
  std::uint64_t _jobs_count;
};

IntrusivePtr<FairThreadPool> MakeFairThreadPool(std::uint64_t threads = yaclib_std::thread::hardware_concurrency());

}  // namespace yaclib
