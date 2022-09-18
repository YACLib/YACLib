#pragma once

/**
 * This Thread Pool inspired on golang and based on tokio implementation
 */
#include <runtime/idle.hpp>
#include <runtime/parker.hpp>
#include <util/global_queue.hpp>
#include <util/work_stealing_queue.hpp>

#include <yaclib/exe/executor.hpp>

#include <cstddef>
#include <deque>
#include <optional>
#include <yaclib_std/atomic>
#include <yaclib_std/thread_local>

namespace yaclib {

constexpr size_t kTickFrequency = 61;

class GolangThreadPool;
class Worker;

YACLIB_THREAD_LOCAL_PTR(Worker) _worker;

class Worker final {
  static constexpr std::size_t kLocalQueueCapacity = 256;
  friend class GolangThreadPool;

 public:
  Worker(GolangThreadPool& host, std::uint16_t index) : _host{host}, _index{index} {
  }

  Worker(Worker&&) = delete;
  Worker& operator=(Worker&&) = delete;
  Worker(const Worker&) = delete;
  Worker& operator=(const Worker&) = delete;

  void Start();

  // Steal from this worker
  Job* TryStealJobs(Worker& steal_from) {
    auto count = steal_from._local_jobs.Grab(_stealed_jobs);
    if (count == 0) {
      return nullptr;
    }
    // TODO(kononovk): optimize this:
    //  1. Push all
    //  2. Grab into _local_jobs
    for (size_t i = 1; i < count; ++i) {
      bool pushed = _local_jobs.TryPush(_stealed_jobs[i]);
      YACLIB_ASSERT(pushed);
    }
    return _stealed_jobs[0];
  }

  bool TryPark();

  void Park();

  void RunJob(Job& job);

  void Tick();

 private:
  // Blocking
  Job* PickNextTask();

  // Run Loop
  void Work();

  static_assert(kLocalQueueCapacity % 2 == 0 && kLocalQueueCapacity != 0);

  // Local queue
  detail::WorkStealingQueue<kLocalQueueCapacity> _local_jobs{};
  std::array<Job*, kLocalQueueCapacity / 2> _stealed_jobs;

  // parent thread pool
  GolangThreadPool& _host;

  Waiter _waiter;

  // worker index in thread pool's workers queue
  const std::uint16_t _index;

  // Worker thread
  yaclib_std::thread _thread;

  std::size_t _tick{0};

  bool _is_shutdown{false};
  bool _is_searching{false};
};

class GolangThreadPool : public IExecutor {
 public:
  friend class Worker;

  [[nodiscard]] Type Tag() const noexcept final {
    return Type::Custom; // TODO
  }


  explicit GolangThreadPool(std::size_t threads) noexcept
    : _threads{threads}, _idle(static_cast<std::uint16_t>(threads)) {
  }

  void Start() {
    for (std::size_t i = 0; i != _threads; ++i) {
      _workers.emplace_back(*this, i);
    }
    for (auto& worker : _workers) {
      worker.Start();
    }
  }

  void NotifyOne() {
    if (auto index = _idle.WorkerToNotify(); index != std::numeric_limits<uint16_t>::max()) {
      _workers[index]._waiter.Unpark();
    }
  }

  /**
   * Return true if executor still alive, that means job passed to submit will be Call
   */
  [[nodiscard]] bool Alive() const noexcept final {
    return _stopped.load() == 0;
  }

  /**
   * Submit given job. This method may either Call or Drop the job
   *
   * Call if executor is Alive, otherwise Drop
   * \param job task to execute
   */
  void Submit(Job& job) noexcept final {
    if (_worker == nullptr || !_worker->_local_jobs.TryPush(&job)) {
      // TODO(kononovk) move half of _local_jobs to _global_task
      _global_tasks.Push(&job);
    }
    NotifyOne();
  }

  /**
   * Wait until all threads joined or idle
   *
   * \note It is blocking
   * \note It can not be called from this thread pool job
   */
  void Wait() noexcept {
    for (auto& worker : _workers) {
      YACLIB_ASSERT(worker._thread.joinable());
      worker._thread.join();
    }
  }

  /**
   * Disable further Submit() calls from being accepted
   *
   * \note after Stop() was called Alive() returns false
   */
  void Stop() noexcept {
    _stopped.store(1);
    for (auto& worker : _workers) {
      worker._waiter.Unpark();
    }
  }

  /**
   * Call Stop() and Drop() waiting tasks
   *
   * \note Drop() can be called here or in thread pool's threads
   */
  void Cancel() noexcept {
    _stopped.store(2);
    for (auto& worker : _workers) {
      worker._waiter.Unpark();
    }
  }

 private:
  std::size_t _threads;
  std::deque<Worker> _workers;

  /// Global task queue used for:
  ///  1. Submit work to the scheduler while **not** currently on a worker thread.
  ///  2. Submit work to the scheduler when a worker run queue is saturated
  detail::GlobalQueue _global_tasks;

  /// Coordinates idle workers
  Idle _idle;
  yaclib_std::atomic_size_t _joined_workers{0};
  yaclib_std::atomic_uint8_t _stopped{0};
};

Job* Worker::PickNextTask() {
  Job* job = nullptr;

  // TODO(kononovk): 0) LIFO slot

  // 1) Periodically global queue
  if (_tick == kTickFrequency) {
    job = _host._global_tasks.TryPopOne();
    if (job != nullptr) {
      return job;
    }
  }

  // 2) Local queue
  job = _local_jobs.TryPop();
  if (job != nullptr) {
    return job;
  }

  // 3) Global queue
  if (_tick != kTickFrequency) {
    job = _host._global_tasks.TryPopOne();
    if (job != nullptr) {
      return job;
    }
  }

  // Transition to searching
  if (!_is_searching) {
    _is_searching = _host._idle.TransitionWorkerToSearching();
    if (!_is_searching) {
      return nullptr;
    }
  }

  // 4) Work stealing
  // TODO(kononovk): think about random start
  for (size_t i = 1; i < _host._threads; ++i) {
    size_t index = (_index + i) % _host._threads;
    YACLIB_ASSERT(index != _index);
    job = TryStealJobs(_host._workers[index]);
    if (job != nullptr) {
      return job;
    }
  }

  // 5) Fallback on checking the global queue
  job = _host._global_tasks.TryPopOne();

  // 6) Park if job == nullptr, because we don't have tasks =(
  return job;
}

void Worker::Work() {
  while (!_is_shutdown) {
    if (_tick == kTickFrequency) {
      _is_shutdown = !_host.Alive();
    }
    Job* next = PickNextTask();
    if (next != nullptr) {
      RunJob(*next);
    } else {
      Park();
    }
    Tick();
  }

  if (_host._joined_workers.fetch_add(1, std::memory_order_acq_rel) == _host._threads - 1) {
    for (size_t index = 0; index < _host._threads; ++index) {
      _host._workers[index]._local_jobs.Drain(_host._stopped.load() == 1);
    }
    _host._global_tasks.Drain(_host._stopped.load() == 1);
  }
}

void Worker::RunJob(Job& job) {
  if (_is_searching) {
    _is_searching = false;
    if (_host._idle.TransitionWorkerFromSearching()) {
      _host.NotifyOne();
    }
  }
  // Run the task
  job.Call();
}

void Worker::Tick() {
  if (_tick++ == kTickFrequency) {
    _tick = 0;
  }
}

bool Worker::TryPark() {
  // return false; if we want busy wait
  YACLIB_ASSERT(_local_jobs.Empty());
  if (!_host._idle.TransitionWorkerToParked(_index, std::exchange(_is_searching, false))) {
    return true;
  }
  for (auto& worker : _host._workers) {
    if (!worker._local_jobs.Empty()) {
      if (_host._idle.UnparkWorkerById(worker._index)) {
        worker._waiter.Unpark();
      }
      return true;
    }
  }
  if (_host._global_tasks.Empty()) {
    return true;
  }
  (void)_host._idle.UnparkWorkerById(_index);
  return false;
}

void Worker::Park() {
  if (!TryPark()) {
    return;
  }
  while (true) {
    _is_shutdown = _waiter.Park([&] {
      return !_host.Alive();
    });
    if (_is_shutdown) {
      break;
    }
    if (!_host._idle.IsParked(static_cast<uint16_t>(_index))) {
      _is_searching = true;
      break;
    }
  }
}

void Worker::Start() {
  _thread = std::thread{[this]() noexcept {
    // SetCurrentThreadPool(_host);
    _worker = this;
    Work();
    _worker = nullptr;
  }};
}

}  // namespace yaclib
