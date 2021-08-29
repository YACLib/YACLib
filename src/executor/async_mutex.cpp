#include <container/mpsc_stack.hpp>

#include <yaclib/config.hpp>
#include <yaclib/executor/executor.hpp>

#include <atomic>
#include <utility>

namespace yaclib::executor {
namespace {

class AsyncMutex : public IExecutor, public ITask {
  // Inheritance from two IRef's, but that's okay, because they are pure virtual
 public:
  explicit AsyncMutex(IExecutorPtr executor) : _executor{std::move(executor)} {
  }

  ~AsyncMutex() override {
    auto nodes{_tasks.TakeAllLIFO()};
    auto task = static_cast<ITask*>(nodes);
    while (task != nullptr) {
      auto next = static_cast<ITask*>(task->_next);
      task->Call();
      task->DecRef();
      task = next;
    }
  }

 private:
  bool Execute(ITask& task) final {
    task.IncRef();
    _tasks.Put(&task);

    if (_work_counter.fetch_add(1, std::memory_order_acq_rel) == 0) {
      _executor->Execute(*this);
    }
    return true;
  }

  void Call() noexcept final {
    auto nodes{_tasks.TakeAllFIFO()};
    size_t size = 0;

    auto task = static_cast<ITask*>(nodes);
    while (task != nullptr) {
      auto next = static_cast<ITask*>(task->_next);
      task->Call();
      task->DecRef();
      task = next;
      ++size;
    }

    if (_work_counter.fetch_sub(size, std::memory_order_acq_rel) > size) {
      _executor->Execute(*this);
    }
  }

  IExecutorPtr _executor;
  container::intrusive::MPSCStack _tasks;
  // TODO remove _work_counter, make active/inactive like libunifex
  alignas(kCacheLineSize) std::atomic<size_t> _work_counter{0};
};

}  // namespace

IExecutorPtr MakeAsyncMutex(IExecutorPtr executor) {
  return new container::Counter<AsyncMutex>{std::move(executor)};
}

}  // namespace yaclib::executor
