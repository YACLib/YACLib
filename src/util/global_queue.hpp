#pragma once

#include <util/intrusive_list.hpp>

#include <yaclib/exe/job.hpp>

#include <span>
#include <yaclib_std/mutex>

namespace yaclib::detail {

/// Unbounded queue shared between workers
class GlobalQueue {
 public:
  /**
   * Push a job into the global queue
   */
  void Push(Job* job) noexcept {
    std::lock_guard lock{_m};
    PushImpl(job);
  }

  /**
   * Offload all jobs into the global queue
   */
  void Offload(std::span<Job*> buffer) noexcept {
    // TODO create queue from buffer and push it
    std::lock_guard lock{_m};
    for (auto* job : buffer) {
      PushImpl(job);
    }
  }

  /**
   * Returns nullptr if queue is empty
   */
  Job* TryPopOne() noexcept {
    std::lock_guard lock{_m};
    if (_jobs.Empty()) {
      return nullptr;
    }
    --_jobs_size;
    return static_cast<Job*>(&_jobs.PopFront());
  }

  /**
   * Returns number of items in `out_buffer`
   */
  size_t Grab(std::span<Job*> out_buffer, size_t workers) noexcept {
    size_t number = 0;
    std::lock_guard lock{_m};
    if (_jobs.Empty() || out_buffer.empty()) {
      return 0;
    }
    size_t to_grab = std::min(_jobs_size / workers, out_buffer.size());
    if (to_grab == 0) {
      to_grab = 1;
    }
    for (; number < to_grab; ++number) {
      out_buffer[number] = static_cast<Job*>(&_jobs.PopFront());
      --_jobs_size;
    }
    return to_grab;
  }

  void Drain(bool soft) noexcept {
    while (auto* job = TryPopOne()) {
      if (soft) {
        job->Call();
      } else {
        job->Drop();
      }
    }
  }

  bool Empty() const noexcept {
    std::lock_guard lock{_m};
    return _jobs.Empty();
  }

 private:
  void PushImpl(Job* item) noexcept {
    _jobs.PushBack(*item);
    ++_jobs_size;
  }

  mutable yaclib_std::mutex _m;
  detail::List _jobs;
  size_t _jobs_size{0};
};

}  // namespace yaclib::detail
