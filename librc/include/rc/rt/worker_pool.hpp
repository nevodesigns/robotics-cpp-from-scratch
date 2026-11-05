// rc/rt/worker_pool.hpp
//
// A pool of workers draining a queue, from lesson 07-10, graduated.
//
// Lessons 07-06 and 07-07 built the two parts of a worker pool without calling it
// one: a queue a thread can sleep on, and a thread that is always joined. This puts
// them together, and the interesting part is the question the pool has to answer
// that neither part could: how many workers?
//
// Everything below was measured on this curriculum's machine, an i7-10850H: 6
// cores with 2 hardware threads each, so std::thread::hardware_concurrency() says 12.
//
// 48 tasks of about 20 ms of real computation each, 960 ms of work in all, by the
// number of workers. A fixed number of loop iterations, calibrated, and not a wait
// for a clock: the first version of this experiment spun until a wall clock deadline,
// so an oversubscribed task finished on time without having been given the CPU, and
// reported speedups of 15 and 20 on 12 hardware threads, which is impossible.
//
//   workers     1      2      4      8     12     24     48
//   speedup   1.0x   2.0x   3.5x   6.1x   6.6x   6.8x   7.2x     (run 1)
//   speedup   1.0x   2.0x   3.8x   6.4x   7.7x   7.4x   7.4x     (run 2)
//
// It scales to the cores and then stops. Twelve workers did not give twelve times:
// six physical cores and their second threads share execution units, and the
// plateau is near 7. More workers than hardware threads gave nothing, and cost
// memory and switching.
//
// The same 48 tasks if they WAIT 20 ms instead, as a task blocked reading a device
// does:
//
//   workers     1      4     12     48
//   speedup   1.0x   4.0x  11.9x  44.6x
//
// A waiting task uses no core, so it scales far past the core count. The right number
// of workers depends on what the tasks do. For computation, about the number of
// physical cores. For waiting, as many as there are things to wait on, within
// reason. A pool sized once and used for both is wrong for one of them.
//
// And what the pool is for, besides the count: 4000 trivial tasks, a thread started
// for each and joined, 122 ms; the same on a pool of 4, 5.4 ms. A thread costs about
// thirty microseconds to start and stop, which is nothing for a task that runs a
// second and everything for one that runs a microsecond.
//
// The design, in the order it is decided:
//
//   - Tasks are std::function<void()>, the queue is lesson 07-06's, the workers are
//     lesson 07-07's, so a worker is always joined and an exception has somewhere
//     to go.
//   - A task that throws is COUNTED and the worker carries on. An exception escaping
//     a worker would end the process (lesson 07-07), and a pool in which one bad
//     task silently kills a worker slowly stops working. It is caught per task, at
//     the one place that runs tasks.
//   - Shutdown is accepting no more, closing the queue, and joining. The queue hands
//     out what it holds before it reports closed, so everything accepted is run.
//   - wait_idle_for answers "is everything I submitted finished?", with a deadline, as
//     every wait in this curriculum has.
//
// The rule that falls out: size the pool for what the tasks do, not for the machine;
// count, do not lose, a task that fails; refuse work after shutdown rather than
// strand it; and never wait without a deadline.

#ifndef RC_RT_WORKER_POOL_HPP
#define RC_RT_WORKER_POOL_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

#include <rc/rt/blocking_queue.hpp>
#include <rc/rt/scoped_thread.hpp>

namespace rc {
namespace rt {

class WorkerPool {
 public:
  using Task = std::function<void()>;
  using Queue = BlockingQueue<Task>;

  // Starts the workers. Each runs worker_loop until the queue is closed.
  explicit WorkerPool(int workers) {
    const int count = workers > 0 ? workers : 1;
    for (int i = 0; i < count; ++i)
      threads_.emplace_back([this] { worker_loop(); });
  }

  ~WorkerPool() { shutdown(); }

  WorkerPool(const WorkerPool&) = delete;
  WorkerPool& operator=(const WorkerPool&) = delete;

  // Accepts a task, or says no once the pool is shutting down. A task refused is
  // never queued, so it is never stranded behind workers that have gone.
  //
  // The decision and the push are one step, under the lock. Had the push come after
  // the lock was released, shutdown could close the queue in between, the workers
  // would drain and leave, and the task would sit in a queue nobody reads, counted
  // as outstanding for ever.
  bool submit(Task task) {
    const std::lock_guard<std::mutex> held(mutex_);
    if (!accepting_) return false;
    ++outstanding_;
    queue_.push(task);
    return true;
  }

  // True when everything submitted so far has finished, false if the limit passed
  // first.
  bool wait_idle_for(std::chrono::milliseconds limit) {
    std::unique_lock<std::mutex> lock(mutex_);
    return idle_.wait_for(lock, limit, [this] { return outstanding_ == 0; });
  }

  // Stops accepting, lets the workers finish what was accepted, and joins them.
  // Safe to call twice.
  void shutdown() {
    {
      const std::lock_guard<std::mutex> held(mutex_);
      accepting_ = false;
    }
    queue_.close();
    for (ScopedThread& thread : threads_) thread.join();
  }

  int workers() const { return static_cast<int>(threads_.size()); }

  // Tasks finished, whether they returned or threw.
  int completed() const {
    const std::lock_guard<std::mutex> held(mutex_);
    return completed_;
  }

  // Tasks that threw.
  int failed() const {
    const std::lock_guard<std::mutex> held(mutex_);
    return failed_;
  }

 private:
  void worker_loop() {
    for (;;) {
      Task task;
      const Queue::Result result = queue_.pop_for(task, std::chrono::milliseconds(100));
      if (result == Queue::Result::kClosed) return;
      if (result == Queue::Result::kTimeout) continue;
      run_task(task);
    }
  }

  // The one place a task is run. An exception stops here: it is counted, and the
  // worker goes on to the next task.
  void run_task(Task& task) {
    bool threw = false;
    try {
      task();
    } catch (...) {
      threw = true;
    }

    const std::lock_guard<std::mutex> held(mutex_);
    if (threw) ++failed_;
    ++completed_;
    --outstanding_;
    if (outstanding_ == 0) idle_.notify_all();
  }

  Queue queue_;
  mutable std::mutex mutex_;
  std::condition_variable idle_;
  bool accepting_ = true;
  int outstanding_ = 0;
  int completed_ = 0;
  int failed_ = 0;
  std::vector<ScopedThread> threads_;
};

}  // namespace rt
}  // namespace rc

#endif  // RC_RT_WORKER_POOL_HPP
