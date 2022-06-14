#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/job.hpp>
#include <yaclib/executor/strand.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <utility>
#include <yaclib_std/atomic>

namespace yaclib {
namespace {

class /*alignas(kCacheLineSize)*/ Strand : public Job, public IExecutor {
  // Inheritance from two IRef's, but that's okay, because they are pure virtual
 public:
  explicit Strand(IExecutorPtr executor) : _executor{std::move(executor)} {
  }

  ~Strand() override {
    YACLIB_DEBUG(_tasks.load(std::memory_order_acquire) != Mark(), "strand in dtor not empty");
  }

 private:
  [[nodiscard]] Type Tag() const final {
    return Type::Strand;
  }

  void Submit(Job& task) noexcept final {
    auto* old = _tasks.load(std::memory_order_relaxed);
    do {
      task.next = old == Mark() ? nullptr : old;
    } while (!_tasks.compare_exchange_weak(old, &task, std::memory_order_acq_rel, std::memory_order_relaxed));
    if (old == Mark()) {
      static_cast<Job&>(*this).IncRef();
      _executor->Submit(*this);
    }
  }

  void Call() noexcept final {
    auto* node = _tasks.exchange(nullptr, std::memory_order_acquire);
    Node* prev = nullptr;
    do {
      auto* next = node->next;
      node->next = prev;
      prev = node;
      node = next;
    } while (node != nullptr);
    do {
      auto* next = prev->next;
      static_cast<Job*>(prev)->Call();
      prev = next;
    } while (prev != nullptr);
    if (_tasks.load(std::memory_order_acquire) != node ||
        !_tasks.compare_exchange_strong(node, Mark(), std::memory_order_acq_rel, std::memory_order_relaxed)) {
      _executor->Submit(*this);
    } else {
      static_cast<Job&>(*this).DecRef();
    }
  }

  void Cancel() noexcept final {
    auto* node = _tasks.exchange(Mark(), std::memory_order_acq_rel);
    do {
      auto* next = node->next;
      static_cast<Job*>(node)->Cancel();
      node = next;
    } while (node != nullptr);
    static_cast<Job&>(*this).DecRef();
  }

  Node* Mark() noexcept {
    return static_cast<Node*>(this);
  }

  IExecutorPtr _executor;
  yaclib_std::atomic<Node*> _tasks{Mark()};
};

}  // namespace

IExecutorPtr MakeStrand(IExecutorPtr executor) {
  return MakeIntrusive<Strand, IExecutor>(std::move(executor));
}

}  // namespace yaclib
