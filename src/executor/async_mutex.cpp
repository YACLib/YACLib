#include <container/mpsc_stack.hpp>

#include <yaclib/executor/executor.hpp>

#include <atomic>
#include <cassert>
#include <utility>

namespace yaclib::executor {
namespace {

class AsyncMutex final : public executor::IExecutor,
                         public std::enable_shared_from_this<AsyncMutex> {
 public:
  explicit AsyncMutex(IExecutorPtr executor) : executor_{std::move(executor)} {
  }

  ~AsyncMutex() final {
    auto nodes{_tasks.TakeAllLIFO()};
    auto task = static_cast<ITask*>(nodes);
    while (task != nullptr) {
      auto next = static_cast<ITask*>(task->_next);
      task->Call();
      task->Release();
      task = next;
    }
  }

  void Execute(ITask& task) final {
    task.Acquire();
    _tasks.Put(&task);

    if (_work_counter.fetch_add(1, std::memory_order_acq_rel) == 0) {
      executor_->Execute([self = shared_from_this()] {
        self->ExecuteTasks();
      });
    }
  }

 private:
  void ExecuteTasks() {
    auto nodes{_tasks.TakeAllFIFO()};
    size_t size = 0;

    auto task = static_cast<ITask*>(nodes);
    while (task != nullptr) {
      auto next = static_cast<ITask*>(task->_next);
      task->Call();
      task->Release();
      task = next;
      ++size;
    }

    if (_work_counter.fetch_sub(size, std::memory_order_acq_rel) > size) {
      executor_->Execute([self = shared_from_this()] {
        self->ExecuteTasks();
      });
    }
  }

  IExecutorPtr executor_;
  container::intrusive::MPSCStack _tasks;
  alignas(64) std::atomic<size_t> _work_counter{0};
  // TODO remove _work_counter, make active/inactive like libunifex
};

}  // namespace

IExecutorPtr MakeAsyncMutex(IExecutorPtr executor) {
  return std::make_shared<AsyncMutex>(std::move(executor));
}

}  // namespace yaclib::executor
