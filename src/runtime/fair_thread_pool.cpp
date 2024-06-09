#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/helper.hpp>

namespace yaclib {

FairThreadPool::FairThreadPool(std::uint64_t threads) : _jobs_count{0} {
  _workers.reserve(threads);
  for (std::uint64_t i = 0; i != threads; ++i) {
    _workers.emplace_back([&] {
      Loop();
    });
  }
}

FairThreadPool::~FairThreadPool() noexcept {
  YACLIB_DEBUG(!_workers.empty(), "You need explicitly join ThreadPool");
}

IExecutor::Type FairThreadPool::Tag() const noexcept {
  return Type::FairThreadPool;
}

bool FairThreadPool::Alive() const noexcept {
  std::lock_guard lock{_m};
  return !WasStop();
}

void FairThreadPool::Submit(Job& job) noexcept {
  std::unique_lock lock{_m};
  if (WasStop()) {
    lock.unlock();
    job.Drop();
    return;
  }
  _jobs.PushBack(job);
  _jobs_count += 4;  // Add Job
  lock.unlock();
  _idle.notify_one();
}

void FairThreadPool::SoftStop() noexcept {
  std::unique_lock lock{_m};
  if (NoJobs()) {
    Stop(std::move(lock));
  } else {
    _jobs_count |= 2U;  // Want Stop
  }
}

void FairThreadPool::Stop() noexcept {
  Stop(std::unique_lock{_m});
}

void FairThreadPool::HardStop() noexcept {
  std::unique_lock lock{_m};
  detail::List jobs{std::move(_jobs)};
  Stop(std::move(lock));
  while (!jobs.Empty()) {
    auto& job = jobs.PopFront();
    static_cast<Job&>(job).Drop();
  }
}

void FairThreadPool::Wait() noexcept {
  for (auto& worker : _workers) {
    worker.join();
  }
  _workers.clear();
}

void FairThreadPool::Loop() noexcept {
  std::unique_lock lock{_m};
  while (true) {
    while (!_jobs.Empty()) {
      auto& job = _jobs.PopFront();
      lock.unlock();
      static_cast<Job&>(job).Call();
      lock.lock();
      _jobs_count -= 4;  // Pop job
    }
    if (NoJobs() && WantStop()) {
      return Stop(std::move(lock));
    }
    if (WasStop()) {
      return;
    }
    _idle.wait(lock);
  }
}

bool FairThreadPool::WasStop() const noexcept {
  return (_jobs_count & 1U) != 0;
}

bool FairThreadPool::WantStop() const noexcept {
  return (_jobs_count & 2U) != 0;
}

bool FairThreadPool::NoJobs() const noexcept {
  return (_jobs_count >> 2U) == 0;
}

void FairThreadPool::Stop(std::unique_lock<yaclib_std::mutex>&& lock) noexcept {
  _jobs_count |= 1U;
  lock.unlock();
  _idle.notify_all();
}

IntrusivePtr<FairThreadPool> MakeFairThreadPool(std::uint64_t threads) {
  return MakeShared<FairThreadPool>(1, threads);
}

}  // namespace yaclib
