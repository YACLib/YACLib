#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/exe/strand.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/helper.hpp>

#include <utility>
#include <yaclib_std/atomic>

namespace yaclib {
namespace {

class Strand : private Job, public IExecutor {
  // Inheritance from two IRef's, but that's okay, because they are pure virtual
 public:
  explicit Strand(IExecutorPtr executor) : _executor{std::move(executor)} {
  }

  ~Strand() override {
    YACLIB_DEBUG(_jobs.load(std::memory_order_relaxed) != Mark(), "Strand not empty in dtor");
  }

 private:
  [[nodiscard]] Type Tag() const noexcept final {
    return Type::Strand;
  }

  void Submit(Job& job) noexcept final {
    auto* expected = _jobs.load(std::memory_order_relaxed);
    do {
      job.next = expected == Mark() ? nullptr : expected;
    } while (!_jobs.compare_exchange_weak(expected, &job, std::memory_order_acq_rel, std::memory_order_relaxed));
    if (expected == Mark()) {
      static_cast<Job&>(*this).IncRef();
      _executor->Submit(*this);
    }
  }

  void Call() noexcept final {
    auto* node = _jobs.exchange(nullptr, std::memory_order_acquire);
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
    if (_jobs.load(std::memory_order_relaxed) == node &&
        _jobs.compare_exchange_strong(node, Mark(), std::memory_order_release, std::memory_order_relaxed)) {
      static_cast<Job&>(*this).DecRef();
    } else {
      _executor->Submit(*this);
    }
  }

  void Drop() noexcept final {
    auto* node = _jobs.exchange(Mark(), std::memory_order_acq_rel);
    do {
      auto* next = node->next;
      static_cast<Job*>(node)->Drop();
      node = next;
    } while (node != nullptr);
    static_cast<Job&>(*this).DecRef();
  }

  Node* Mark() noexcept {
    return static_cast<Node*>(this);
  }

  IExecutorPtr _executor;
  yaclib_std::atomic<Node*> _jobs{Mark()};
};

}  // namespace

IExecutorPtr MakeStrand(IExecutorPtr executor) {
  return MakeShared<Strand>(1, std::move(executor));
}

}  // namespace yaclib
