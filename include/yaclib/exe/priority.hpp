#pragma once

#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/job.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>
#include <yaclib_std/mutex>

namespace yaclib {

/**
 *
 */
class PriorityExecutor : private Job {
  class PriorityExecutorImpl : public IExecutor {
    [[nodiscard]] Type Tag() const noexcept final;
    [[nodiscard]] bool Alive() const noexcept final;
  };

 public:
  IExecutor& GetExecutor(std::size_t priority);

  void Submit(std::size_t priority, Job& job) {
    {
      std::lock_guard lock{_mutex};
      _queue.emplace_back(priority, &job);
      std::push_heap(_queue.begin(), _queue.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.priority < rhs.priority;
      });
    }
    _executor->Submit(*this);
  }

 private:
  IExecutorPtr _executor;

  void Call() noexcept final {
    Job* job;
    {
      std::lock_guard lock{_mutex};
      std::pop_heap(_queue.begin(), _queue.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.priority < rhs.priority;
      });
      job = _queue.back().job;
      _queue.pop_back();
    }
    job->Call();
  }

  void Drop() noexcept final {
    std::lock_guard lock{_mutex};
    for (auto& entry : _queue) {
      entry.job->Drop();
    }
  }

  struct PriorityJob {
    std::size_t priority;
    Job* job;
  };
  yaclib_std::mutex _mutex;
  std::vector<PriorityJob> _queue;
};

IExecutorPtr MakePriority(IExecutorPtr e);

}  // namespace yaclib
