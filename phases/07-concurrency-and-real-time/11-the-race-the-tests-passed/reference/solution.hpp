#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <atomic>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

// Lesson 07-10's worker pool had a race in its first draft, and the tests passed.
// submit counted a task and pushed it as two steps with the lock released in between,
// so shutdown could close the queue in the gap and strand the task. The window is a few
// instructions wide.
//
// How hard is that to find by running it? Measured, gcc 11, reduced to the essential
// pattern, two real threads, one submitting four tasks and one calling shutdown, and
// 20000 trials at each of three delays, three runs:
//
//   the buggy version stranded a task in    11  28  22      2   8   7      9  18  14
//   of 20000 trials, per setting and run
//   the fixed version stranded a task in     0 of 180000
//
// So running it a lot does find it: about 0.07 percent of trials, on average, and as
// few as 2 in 20000 in a bad run. A suite that runs the race 100 times, which is a lot
// for a test, sees nothing about 93 percent of the time with the bug still in. That is
// how it got through, and a clean run proves nothing, as lessons 07-01 and 07-09
// measured twice already.
//
// The other approach is to stop hoping the threads line up and MAKE them. Name the
// places in the code where an interleaving could matter, and let a test run an action
// at one of them. A hook at the gap that calls shutdown() puts shutdown() exactly where
// the bug needs it, on one thread, every time, with no timing involved.
//
//   race_point("submit.gap");           in the code under test: a name, and nothing else
//   RaceHook at("submit.gap", [&] { intake.shutdown(); });       in the test
//
// With nothing installed, a race point is one atomic load that finds nothing to do.
// With a hook installed, the action runs on whichever thread reached the point, inside
// the call, between the two lines the bug lives between.
//
// And then try EVERY point, not only the one you suspect. A submit with three points can
// have shutdown run before it, in the middle, or after it, and those are all the
// interleavings of the two steps. Measured below: the buggy version strands a task at
// the gap and nowhere else, and the fixed version at none, which is a small exhaustive
// search and not a hope.
//
// One rule, measured, because breaking it hangs. A point placed while a lock is held,
// with a hook that takes the same lock, waits for itself: a 3 second timeout killed
// that program. Place points where no lock is held, and have hooks call the public
// operations, which take their own locks.
//
// What this does not do: it finds the interleavings you thought to name. It is the
// partner of the stress run and not a replacement, and of lesson 07-09's level check,
// which finds the orders you did not.

// A named place in code where a test may run an action. With nothing installed it
// does one atomic load and returns.
//
// The hooks live in a table behind a mutex, and the count of installed hooks is
// atomic so that the common case never takes the mutex.
inline std::atomic<int>& hooks_installed() {
  static std::atomic<int> count{0};
  return count;
}

struct HookEntry {
  std::string name;
  std::function<void()> action;
  int id;
};

inline std::mutex& hook_mutex() {
  static std::mutex mutex;
  return mutex;
}

inline std::vector<HookEntry>& hook_table() {
  static std::vector<HookEntry> table;
  return table;
}

inline void race_point(const char* name) {
  // Nothing installed: the whole cost of a race point in ordinary running.
  if (hooks_installed().load(std::memory_order_acquire) == 0) return;

  // Copy the action out and run it AFTER the table's lock is released. The action
  // may itself reach another race point, and would otherwise wait for itself.
  std::function<void()> action;
  {
    const std::lock_guard<std::mutex> held(hook_mutex());
    for (const HookEntry& entry : hook_table()) {
      if (entry.name == name) {
        action = entry.action;
        break;
      }
    }
  }
  if (action) action();
}

// Installs an action at a named point for the length of a scope.
class RaceHook {
 public:
  RaceHook(const char* name, std::function<void()> action) {
    const std::lock_guard<std::mutex> held(hook_mutex());
    static int next_id = 1;
    id_ = next_id++;
    hook_table().push_back(HookEntry{name, std::move(action), id_});
    hooks_installed().fetch_add(1, std::memory_order_release);
  }

  ~RaceHook() {
    const std::lock_guard<std::mutex> held(hook_mutex());
    std::vector<HookEntry>& table = hook_table();
    for (std::size_t i = 0; i < table.size(); ++i) {
      if (table[i].id == id_) {
        table.erase(table.begin() + static_cast<std::ptrdiff_t>(i));
        break;
      }
    }
    hooks_installed().fetch_sub(1, std::memory_order_release);
  }

  RaceHook(const RaceHook&) = delete;
  RaceHook& operator=(const RaceHook&) = delete;

 private:
  int id_ = 0;
};

// ---- The code under test. Do not edit it. ----

// A small intake, with the shape that went wrong in lesson 07-10's first draft. A
// task is accepted, counted, and queued; shutdown stops accepting and processes what is
// queued, once. Anything queued after that is never processed, which is stranded.
//
// fixed == false is the first draft: the lock is released between counting and queueing.
// fixed == true is the repair: the decision and the push are one step.
class Intake {
 public:
  explicit Intake(bool fixed) : fixed_(fixed) {}

  bool submit(int id) {
    race_point("submit.before");

    if (fixed_) {
      {
        const std::lock_guard<std::mutex> held(mutex_);
        if (!accepting_) return false;
        items_.push_back(id);
      }
    } else {
      {
        const std::lock_guard<std::mutex> held(mutex_);
        if (!accepting_) return false;
        ++counted_;
      }
      race_point("submit.gap");
      {
        const std::lock_guard<std::mutex> held(mutex_);
        items_.push_back(id);
      }
    }

    race_point("submit.after");
    return true;
  }

  // Stops accepting and processes what is queued, once.
  void shutdown() {
    const std::lock_guard<std::mutex> held(mutex_);
    accepting_ = false;
    shutdown_done_ = true;
    items_.clear();
  }

  // Tasks still queued after shutdown has already run: accepted, and never to be
  // processed. A task that is merely waiting, because shutdown has not run yet, is not
  // stranded, so this is zero until shutdown happens.
  int stranded() const {
    const std::lock_guard<std::mutex> held(mutex_);
    return shutdown_done_ ? static_cast<int>(items_.size()) : 0;
  }

 private:
  bool fixed_;
  mutable std::mutex mutex_;
  bool accepting_ = true;
  bool shutdown_done_ = false;
  int counted_ = 0;
  std::deque<int> items_;
};

#endif  // LESSON_SOLUTION_HPP
