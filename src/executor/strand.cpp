#include <util/mpsc_stack.hpp>

#include <yaclib/config.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/strand.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/fault/atomic.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstdint>
#include <utility>

namespace yaclib {
namespace {

class Strand : public IExecutor, public ITask {
  // Inheritance from two IRef's, but that's okay, because they are pure virtual
 public:
  explicit Strand(IExecutorPtr executor) : _executor{std::move(executor)} {
  }

  ~Strand() override {
    auto* nodes = _tasks.TakeAllLIFO();
    auto* task = static_cast<ITask*>(nodes);
    while (task != nullptr) {
      auto* next = static_cast<ITask*>(task->next);
      task->Cancel();
      task = next;
    }
  }

 private:
  [[nodiscard]] Type Tag() const final {
    return Type::Strand;
  }

  void Submit(ITask& task) noexcept final {
    _tasks.Put(task);

    if (_work_counter.fetch_add(1, std::memory_order_acq_rel) == 0) {
      static_cast<ITask&>(*this).IncRef();
      _executor->Submit(*this);
    }
  }

  void Call() noexcept final {
    auto* nodes = _tasks.TakeAllFIFO();
    std::int32_t size = 0;

    auto* task = static_cast<ITask*>(nodes);
    while (task != nullptr) {
      auto* next = static_cast<ITask*>(task->next);
      task->Call();
      task = next;
      ++size;
    }

    if (_work_counter.fetch_sub(size, std::memory_order_acq_rel) > size) {
      _executor->Submit(*this);
    } else {
      static_cast<ITask&>(*this).DecRef();
    }
  }

  void Cancel() noexcept final {
    static_cast<ITask&>(*this).DecRef();
  }

  IExecutorPtr _executor;
  detail::MPSCStack _tasks;
  // TODO remove _work_counter, make active/inactive like libunifex
  alignas(kCacheLineSize) yaclib_std::atomic_int32_t _work_counter{0};
};

}  // namespace

IExecutorPtr MakeStrand(IExecutorPtr executor) {
  return MakeIntrusive<Strand, IExecutor>(std::move(executor));
}

}  // namespace yaclib
