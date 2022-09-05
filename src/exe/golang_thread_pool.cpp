/**
 * This Thread Pool inspired on golang and based on tokio implementation
 */
#include <exe/idle.hpp>
#include <exe/parker.hpp>
#include <util/global_queue.hpp>
#include <util/metrics.hpp>
#include <util/work_stealing_queue.hpp>

#include <yaclib/exe/thread_factory.hpp>
#include <yaclib/exe/thread_pool.hpp>

#include <cstddef>
#include <deque>
#include <iostream>
#include <optional>
#include <random>
#include <yaclib_std/atomic>
#include <yaclib_std/thread_local>

namespace yaclib {
namespace {

constexpr size_t kTickFrequency = 61;

class GolangThreadPool;
class Worker;
YACLIB_THREAD_LOCAL_PTR(Worker) _worker;

class Worker {
  static constexpr size_t kLocalQueueCapacity = 256;

 public:
  Worker(GolangThreadPool& host, size_t index) : _host(host), _index(index) {
  }

  Worker(Worker&&) = delete;
  Worker& operator=(Worker&&) = delete;
  Worker(const Worker&) = delete;
  Worker& operator=(const Worker&) = delete;

  void Start() {
    _thread.emplace([this]() noexcept {
      _worker = this;
      Work();
      _worker = nullptr;
    });
  }

  // Submit job
  bool TrySubmit(Job& task) noexcept {
    // TODO(kononovk): add LIFO slot
    return _local_jobs.TryPush(&task);
  }

  void Join() {
    _thread->join();  // NOLINT(bugprone-unchecked-optional-access)
  }

  // Steal from this worker
  Job* TryStealJobs(Worker& steal_from) {
    auto count = steal_from._local_jobs.Grab(_stealed_jobs);
    if (count == 0) {
      return nullptr;
    }

    for (size_t i = 1; i < count; ++i) {
      bool pushed = _local_jobs.TryPush(_stealed_jobs[i]);
      YACLIB_ASSERT(pushed);
    }
    return _stealed_jobs[0];
  }

  // Wake parked worker
  void Wake() {
  }

  void Park();

  void Tick();

  static Worker* Current() {
    return _worker;
  }

  detail::WorkerMetrics Metrics() const {
    return _metrics;
  }

  GolangThreadPool& Host() const {
    return _host;
  }

 private:
  void OffloadTasksToGlobalQueue(Job* overflow) {
  }

  // Blocking
  Job* PickNextTask();

  // Clear tasks
  void Clear();

  // Run Loop
  void Work();

  static_assert(kLocalQueueCapacity % 2 == 0 && kLocalQueueCapacity != 0);

  // Local queue
  detail::WorkStealingQueue<kLocalQueueCapacity> _local_jobs{};
  std::array<Job*, kLocalQueueCapacity / 2> _stealed_jobs;

  // parent thread pool
  GolangThreadPool& _host;

  // worker index in thread pool's workers queue
  const size_t _index;

  // LIFO slot
  [[maybe_unused]] Job* _lifo_slot{nullptr};  // TODO(kononovk)

  // Worker thread
  std::optional<yaclib_std::thread> _thread{};

  // For work stealing
  std::mt19937_64 _twister{};  // NOLINT

  // Parking lot
  yaclib_std::atomic<uint32_t> _wakeups{};

  std::size_t _tick{0};

  detail::WorkerMetrics _metrics;  // TODO(kononovk): add metrics

  bool _is_shutdown{false};
};

class GolangThreadPool : public IThreadPool {
 public:
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

  [[nodiscard]] Type Tag() const noexcept final {
    return IExecutor::Type::GolangThreadPool;
  }

  /**
   * Return true if executor still alive, that means job passed to submit will be Call
   */
  [[nodiscard]] bool Alive() const noexcept final {
    return !_stopped.load();
  }

  /**
   * Submit given job. This method may either Call or Drop the job
   *
   * Call if executor is Alive, otherwise Drop
   * \param task task to execute
   */
  void Submit(Job& task) noexcept final {
    if (_worker == nullptr || !_worker->TrySubmit(task)) {
      _global_tasks.Push(&task);
    }
  }

  /**
   * Wait until all threads joined or idle
   *
   * \note It is blocking
   * \note It can not be called from this thread pool job
   */
  void Wait() noexcept final {
    std::terminate();
  }

  /**
   * Disable further Submit() calls from being accepted
   *
   * \note after Stop() was called Alive() returns false
   */
  void Stop() noexcept final {
    _stopped.store(true);
    for (auto& worker : _workers) {
      worker.Join();
    }
  }

  /**
   * Call Stop() and Drop() waiting tasks
   *
   * \note Drop() can be called here or in thread pool's threads
   */
  void Cancel() noexcept final {
    std::terminate();
  }
  friend class Worker;

 private:
  std::size_t _threads;
  std::deque<Worker> _workers;

  /// Global task queue used for:
  ///  1. Submit work to the scheduler while **not** currently on a worker thread.
  ///  2. Submit work to the scheduler when a worker run queue is saturated
  detail::GlobalQueue _global_tasks;

  /// Coordinates idle workers
  Idle _idle;

  yaclib_std::atomic_bool _stopped{false};
  yaclib_std::atomic_size_t _joined_workers{0};
};

// TODO(kononovk): 0) LIFO slot
Job* Worker::PickNextTask() {
  Job* job = nullptr;

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

  // 4) Work stealing
  for (size_t i = 1; i < _host._threads; ++i) {
    size_t index = (_index + i) % _host._threads;
    YACLIB_ASSERT(index != _index);
    // TODO(kononovk): Maybe steal randomly to minimize contention ??
    job = TryStealJobs(_host._workers[index]);
    if (job != nullptr) {
      return job;
    }
  }

  // 5) Park, no tasks =(
  return nullptr;
}

void Worker::Clear() {
  _local_jobs.Clear();
  // TODO(kononovk): LIFO slot
}

void Worker::Work() {
  while (!_is_shutdown) {
    if (_tick == kTickFrequency) {
      _is_shutdown = !_host.Alive();
    }
    Job* next = PickNextTask();
    if (next != nullptr) {
      next->Call();
    } else {
      // Park woker
      Park();
    }
    Tick();
  }

  if (_host._joined_workers.fetch_add(1, std::memory_order_acq_rel) == _host._threads - 1) {
    for (size_t index = 0; index < _host._threads; ++index) {
      _host._workers[index].Clear();
    }
    _host._global_tasks.Clear();
  }
}

void Worker::Tick() {
  if (_tick++ == kTickFrequency) {
    _tick = 1;
  }
}

void Worker::Park() {
  bool transition_to_parked = false;

  // Workers should not park if they have work to do
  // TODO(kononovk): add also LIFO slot checking
  YACLIB_ASSERT(_local_jobs.Empty());
  if (!_local_jobs.Empty()) {
    transition_to_parked = false;
  }
  // TODO: add transition_to_parked and transition_to_searching methods from rust

  if (transition_to_parked) {
    while (_is_shutdown) {
      /*core = self.park_timeout(core, None);

       // Run regularly scheduled maintenance
       core.maintenance(&self.worker);

       if (transition_from_parked()) {
           break;
         }
     }*/
    }

    /*if let Some(f) = &self.worker.shared.config.after_unpark {
        f();
      }
    core*/
  }
}

}  // namespace

IThreadPoolPtr MakeGolangThreadPool(std::size_t threads) {
  auto tp = MakeShared<GolangThreadPool>(1, threads);
  tp->Start();
  return tp;
}

}  // namespace yaclib
