#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <atomic>
#include <chrono>
#include <cstddef>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

// A flag tells a thread to stop. Lesson 07-01 built one, and it has two limits that this
// lesson measures: it cannot wake a thread that is asleep, and it says nothing to anything
// that is waiting on something else. C++20 has std::stop_token, std::stop_source and
// std::stop_callback for this. This curriculum is C++17, so they are built by hand, which is
// the way to learn what they are for.
//
// Measured on this curriculum's machine, gcc 11, 40 trials each. How long a worker took to
// stop after the request, by how it was waiting:
//
//   a flag polled between 10 ms sleeps      median 3.27 ms   worst 9.3 ms
//   a wait the request itself wakes         median 72 us     worst 105 to 122 us
//
// Polling is not wrong, it is slow, and it is slow in proportion to its sleep. A wait that
// the request wakes is about 45 times quicker at the median and 80 at the worst, because
// nothing has to be noticed: the request does the waking.
//
// And a thread that is computing, not waiting, stops as fast as it LOOKS. A loop that checks
// the flag every N iterations, 15 trials each:
//
//   checked every 1 iteration         median 0.9 us     worst 2.5 us
//   checked every 1000 iterations     median 2.1 us     worst 3.7 us (24 in a second run)
//   checked every 100000 iterations   median 80 to 240 us   worst 313 to 355 us
//
// How often a loop checks is how fast it can be stopped, and the suite shows the same thing
// in counted iterations, which are exact. A task that never checks cannot be stopped at all.
// That is what "cooperative" means, and why nothing here can cancel a task that does not
// look.
//
// The three pieces, and what each is for:
//
//   StopSource   the end that asks. request_stop() says true once, to whoever asked first.
//   StopToken    the end that looks. A cheap copy of a handle, shared, safe to hand to any
//                number of threads.
//   StopCallback a function to run when the request comes, so a thread blocked on a queue
//                or a condition variable is WOKEN by it. It runs at once if the request
//                has already been made, so registering late cannot miss it.
//
// With a callback, stopping a queue's consumers is one line: a StopCallback that calls
// queue.close(). Lesson 07-06's close() wakes every waiter, and now the request does it.
//
// The careful part. A callback may be running on the requesting thread at the moment the
// object that registered it is being destroyed, and what it captured is about to go away. Left
// alone that is a use of freed memory. So the destructor must WAIT for a callback that is in
// progress. Measured in the suite: a callback held open, a destructor started on another
// thread, and the destructor did not finish until the callback did. The wait has a cost, a
// callback must not destroy its own StopCallback, and this is what std::stop_callback does.
//
// The rule that falls out: ask with a source, look with a token, wake sleepers with a
// callback, check at the granularity you can afford to be stopped at, and never destroy a
// callback's captures before its destructor has returned.

struct StopState {
  std::atomic<bool> stopped{false};
  std::mutex mutex;        // guards callbacks and next_id
  std::mutex run_mutex;    // held while callbacks run, so a destructor can wait for them
  struct Entry {
    int id;
    std::function<void()> action;
  };
  std::vector<Entry> callbacks;
  int next_id = 1;
};

class StopSource;
class StopCallback;

// The end that looks. A copy of a handle: cheap, and safe to give to any thread.
class StopToken {
 public:
  StopToken() = default;

  bool stop_requested() const { return state_ && state_->stopped.load(std::memory_order_acquire); }

  // False for a token that came from no source, which can never be asked to stop.
  bool stop_possible() const { return state_ != nullptr; }

 private:
  friend class StopSource;
  friend class StopCallback;
  explicit StopToken(std::shared_ptr<StopState> state) : state_(std::move(state)) {}
  std::shared_ptr<StopState> state_;
};

// The end that asks.
class StopSource {
 public:
  StopSource() : state_(std::make_shared<StopState>()) {}

  StopToken get_token() const { return StopToken(state_); }

  // TODO 1: ask every holder of a token to stop, and run the callbacks.
  //
  //   - if (state_->stopped.exchange(true, std::memory_order_acq_rel)) return false;
  //     Only the first call goes on. Later ones return false and do nothing.
  //   - Lock state_->run_mutex FIRST, with a std::lock_guard, and keep it for the whole run.
  //   - Then lock state_->mutex in a block of its own, copy state_->callbacks into a local
  //     vector, and leave the block.
  //   - Outside that lock, call entry.action() for each entry in the copy.
  //   - Return true.
  //
  // run_mutex is taken before the copy so that a destructor, which removes its callback and
  // then waits on run_mutex, either finds the list without it or waits for this run. If the
  // copy came first there would be a moment when a callback is about to run and its owner
  // has already gone.
  //
  // The stub asks nobody, which is the safe direction to be wrong in.
  bool request_stop() { return false; }

 private:
  std::shared_ptr<StopState> state_;
};

// A function to run when stop is requested: at once if it already has been.
class StopCallback {
 public:
  // TODO 2: register the action.
  //
  //   - If state_ is null there is nothing to register with: return.
  //   - Lock state_->mutex in a block of its own. If state_->stopped is already true,
  //     remember to run the action now. Otherwise take the next id (id_ = state_->next_id++)
  //     and push StopState::Entry{id_, action} onto state_->callbacks.
  //   - AFTER the block, outside the lock, run the action if it was to be run now.
  //
  // Checking stopped under the mutex is what makes registering late safe: a request that
  // arrives first runs this action here, and one that arrives later finds it in the list.
  // Exactly once either way.
  StopCallback(const StopToken& token, std::function<void()> action) : state_(token.state_) {
    (void)action;
  }

  // TODO 3: deregister, and wait for a callback that is running.
  //
  //   - If state_ is null or id_ is 0 there is nothing to do.
  //   - Lock state_->mutex and erase the entry whose id equals id_.
  //   - Then lock state_->run_mutex with a std::lock_guard, and let it go. That waits for a
  //     request that is running callbacks now. What the action captured is about to be
  //     destroyed by whoever owns this object, so it must not be running.
  ~StopCallback() {}

  StopCallback(const StopCallback&) = delete;
  StopCallback& operator=(const StopCallback&) = delete;

 private:
  std::shared_ptr<StopState> state_;
  int id_ = 0;
};

// TODO 4: sleep for the duration unless stop is requested first. True if it was stopped.
//
//   - If token.stop_requested() return true at once.
//   - A local std::mutex, std::condition_variable and bool signalled, in that order.
//   - A StopCallback declared AFTER them, so it is destroyed before them, whose action sets
//     signalled under the mutex and calls notify_all.
//   - A std::unique_lock, then woken.wait_for(lock, duration, [&] { return signalled; }).
//   - Return signalled || token.stop_requested().
//
// The stub returns false at once without sleeping.
inline bool sleep_unless_stopped(const StopToken& token, std::chrono::milliseconds duration) {
  (void)token;
  (void)duration;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
