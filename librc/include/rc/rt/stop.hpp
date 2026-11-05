// rc/rt/stop.hpp
//
// Asking work to stop, from lesson 07-14, graduated.
//
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

#ifndef RC_RT_STOP_HPP
#define RC_RT_STOP_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace rc {
namespace rt {

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

  // Asks every holder of a token to stop and runs the registered callbacks. True for the one
  // call that made the request, false for any after it, which do nothing.
  bool request_stop() {
    if (state_->stopped.exchange(true, std::memory_order_acq_rel)) return false;

    // run_mutex is taken BEFORE the callbacks are copied, so a destructor that removes its
    // callback and then takes run_mutex either finds the list without it or waits for
    // this run to finish. There is no moment when a callback is about to run and its owner
    // has already gone.
    const std::lock_guard<std::mutex> running(state_->run_mutex);
    std::vector<StopState::Entry> to_run;
    {
      const std::lock_guard<std::mutex> held(state_->mutex);
      to_run = state_->callbacks;
    }
    for (const StopState::Entry& entry : to_run) entry.action();
    return true;
  }

 private:
  std::shared_ptr<StopState> state_;
};

// A function to run when stop is requested: at once if it already has been.
class StopCallback {
 public:
  StopCallback(const StopToken& token, std::function<void()> action) : state_(token.state_) {
    if (!state_) return;

    bool run_now = false;
    {
      const std::lock_guard<std::mutex> held(state_->mutex);
      if (state_->stopped.load(std::memory_order_acquire)) {
        run_now = true;
      } else {
        id_ = state_->next_id++;
        state_->callbacks.push_back(StopState::Entry{id_, action});
      }
    }
    // Outside the lock: the action may itself touch the source or the queue it closes.
    if (run_now) action();
  }

  ~StopCallback() {
    if (!state_ || id_ == 0) return;
    {
      const std::lock_guard<std::mutex> held(state_->mutex);
      for (std::size_t i = 0; i < state_->callbacks.size(); ++i) {
        if (state_->callbacks[i].id == id_) {
          state_->callbacks.erase(state_->callbacks.begin() + static_cast<std::ptrdiff_t>(i));
          break;
        }
      }
    }
    // If a request is running callbacks right now, wait for it. What the action captured
    // is about to be destroyed by whoever owns this object.
    const std::lock_guard<std::mutex> wait(state_->run_mutex);
  }

  StopCallback(const StopCallback&) = delete;
  StopCallback& operator=(const StopCallback&) = delete;

 private:
  std::shared_ptr<StopState> state_;
  int id_ = 0;
};

// Sleeps for the duration unless stop is requested first. True if it was stopped.
//
// A wait the request wakes, not a sleep that is polled: the callback sets a flag and notifies,
// so the sleeper leaves the moment the request is made.
inline bool sleep_unless_stopped(const StopToken& token, std::chrono::milliseconds duration) {
  if (token.stop_requested()) return true;

  std::mutex mutex;
  std::condition_variable woken;
  bool signalled = false;

  // Declared after the three above, so destroyed before them: the callback cannot outlive
  // what it captured.
  const StopCallback wake(token, [&] {
    {
      const std::lock_guard<std::mutex> held(mutex);
      signalled = true;
    }
    woken.notify_all();
  });

  std::unique_lock<std::mutex> lock(mutex);
  woken.wait_for(lock, duration, [&] { return signalled; });
  return signalled || token.stop_requested();
}

}  // namespace rt
}  // namespace rc

#endif  // RC_RT_STOP_HPP
