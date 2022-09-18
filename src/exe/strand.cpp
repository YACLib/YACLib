#include <yaclib/exe/strand.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/helper.hpp>

#include <utility>

namespace yaclib {

Strand::Strand(IExecutorPtr e) noexcept : _executor{std::move(e)} {
}

Strand::~Strand() noexcept {
  YACLIB_DEBUG(_jobs.load(std::memory_order_relaxed) != Mark(), "Strand not empty in dtor");
}

IExecutor::Type Strand::Tag() const noexcept {
  return Type::Strand;
}

bool Strand::Alive() const noexcept {
  return _executor->Alive();
}

void Strand::Submit(Job& job) noexcept {
  auto* expected = _jobs.load(std::memory_order_relaxed);
  do {
    job.next = expected == Mark() ? nullptr : expected;
  } while (!_jobs.compare_exchange_weak(expected, &job, std::memory_order_acq_rel, std::memory_order_relaxed));
  if (expected == Mark()) {
    static_cast<Job&>(*this).IncRef();
    _executor->Submit(*this);
  }
}

void Strand::Call() noexcept {
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

void Strand::Drop() noexcept {
  auto* node = _jobs.exchange(Mark(), std::memory_order_acq_rel);
  do {
    auto* next = node->next;
    static_cast<Job*>(node)->Drop();
    node = next;
  } while (node != nullptr);
  static_cast<Job&>(*this).DecRef();
}

detail::Node* Strand::Mark() noexcept {
  return static_cast<Node*>(this);
}

IExecutorPtr MakeStrand(IExecutorPtr e) {
  return MakeShared<Strand>(1, std::move(e));
}

}  // namespace yaclib
