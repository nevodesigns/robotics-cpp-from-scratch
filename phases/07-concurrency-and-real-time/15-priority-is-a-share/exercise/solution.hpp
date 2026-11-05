#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

// A priority is a promise about who goes first, and two things about it are not what they sound like:
// it is a SHARE and not an order of politeness, and it can invert.
//
// Measured on this curriculum's machine, Linux, two CPU-bound processes pinned to ONE core with
// taskset, 3 seconds, millions of loop iterations each:
//
//   nice 0 against nice 0       748    and   751      an even split of the 1489 it did alone
//   nice 0 against nice 10      1330   and   145      90 percent and 10
//   nice 0 against nice 19      1448   and   24       98.4 percent and 1.6
//
// A lower priority is not "runs a little less". At nice 19 the process got 1.6 percent of the core. The kernel
// gives each nice value a weight, nice 0 is 1024, nice 10 is 110, nice 19 is 15, and each process gets its weight
// over the sum. 1024 over 1134 is 90.3 percent and 1024 over 1039 is 98.6, against the measured 90.2 and 98.4. The
// arithmetic predicted the measurement, which is why the suite can test the arithmetic.
//
// And an ordinary program cannot ask for more. Measured, unprivileged:
//
//   sched_setscheduler(SCHED_FIFO, 10)    refused: Operation not permitted     (priorities run 1 to 99)
//   nice(-5), a higher priority           refused: Operation not permitted
//
// So a robot that needs real time priorities needs the privilege to have them, set up by whoever installs
// it, and a lesson cannot assume it. What follows is therefore a simulation, which also makes it exact.
//
// Priority inversion. Three tasks and one lock. Low (1) takes the lock. High (3) wakes and needs it, so it
// waits for Low. Medium (2) wakes: it does not need the lock, and it outranks Low, so it runs, and keeps
// running, because it outranks Low. Low cannot finish the work that would release the lock for as long as
// Medium has something to do. High is waiting for Medium, which it outranks, by way of Low, which it was
// never supposed to wait for.
//
// Simulated, a tick at a time, with the trace of who ran each tick:
//
//   without inheritance   LLHMMMMMMMMMMLLLHL     High finished at tick 16, 15 ticks after it woke
//   with inheritance      LLHLLLHMMMMMMMMMML     High finished at tick 6, 5 ticks after it woke
//
// The fix is priority INHERITANCE: a task holding a lock runs at the highest priority of any task waiting
// for it. Low, holding the lock High needs, runs as if it were High, finishes, releases, and High goes. Medium
// pays: it finished at tick 16 instead of 12, four ticks later, which is the cost and the point. This is the
// bug that reset the Mars Pathfinder lander in 1997, and the fix that was uploaded to it was turning
// inheritance on.
//
// The second scenario is why the rule has to be TRANSITIVE. High waits for a lock Medium holds, and Medium is
// itself waiting for a lock Low holds, with a busy task between them in priority:
//
//   without inheritance   LLNNNNNNNNNNLLMH       High finished at tick 15, after the busy task at 11
//   inheriting directly   the busy task still outranks the Low task, which inherits only Medium's priority
//   transitively          LLLLMHNNNNNNNNNN       High finished at tick 5, the busy task at 15
//
// Low has to inherit from High THROUGH Medium. Direct inheritance only gives Low Medium's priority, which is
// still below the busy task, and the inversion survives.
//
// The rule that falls out: a priority is a share, ask for the privilege you need and expect not to get it,
// inherit through the whole chain of waiters, and measure how long the highest priority task waited, not
// how fast the average one ran.

enum class Op { kCompute, kLock, kUnlock };

// One step of a task's script. kCompute takes `arg` ticks; kLock and kUnlock take no time and name lock `arg`.
struct Step {
  Op op;
  int arg;
};

struct Task {
  char name;
  int priority;      // higher number runs first
  int arrival;       // the tick it becomes runnable
  std::vector<Step> script;

  // The simulation's own bookkeeping.
  std::size_t step = 0;
  int remaining = 0;
  bool done = false;
  int blocked_on = -1;   // the lock it is waiting for, or -1
  int finished_at = -1;  // the last tick it ran
};

struct Result {
  std::string trace;            // who ran each tick, '.' for none
  std::vector<int> finished;    // each task's last tick, in the order given
};

// The Linux weight of each nice value from -20 to 19. Nice 0 is 1024.
const int kNiceWeights[40] = {88761, 71755, 56483, 46273, 36291, 29154, 23254, 18705, 14949, 11916,
                              9548,  7620,  6100,  4904,  3906,  3121,  2501,  1991,  1586,  1277,
                              1024,  820,   655,   526,   423,   335,   272,   215,   172,   137,
                              110,   87,    70,    56,    45,    36,    29,    23,    18,    15};

inline int weight_of(int nice) {
  const int clamped = std::max(-20, std::min(19, nice));
  return kNiceWeights[clamped + 20];
}

// TODO 1: the share of one core a process at nice_a gets against one at nice_b, both always ready.
//
// Its weight over the sum of the two: weight_of(nice_a) / (weight_of(nice_a) + weight_of(nice_b)), as doubles.
// weight_of is given. The suite checks it against the shares the kernel actually gave: 90.3 percent at nice 0
// against 10, which was measured at 90.2.
//
// The stub says half, which is the right answer only when they are equal.
inline double cpu_share(int nice_a, int nice_b) {
  (void)nice_a;
  (void)nice_b;
  return 0.5;
}

// TODO 2: the priority task `index` actually runs at.
//
//   best = tasks[index].priority
//   if inheritance is off, or depth > 8, return best.
//   Otherwise look at every task `other` that is not done and has blocked_on >= 0. If holder[blocked_on] is
//   `index`, that task is waiting for a lock this one holds, so
//       best = max(best, effective_priority(tasks, holder, other, inheritance, depth + 1))
//
// The recursion is the point. A waiter may itself hold a lock that something else waits for, so the priority
// has to travel along the whole chain: High waits for Medium, Medium waits for Low, and Low must run at High's
// priority, not Medium's. The second scenario in the suite is built to fail an answer that looks only one step.
//
// The stub is the task's own priority, which is what no inheritance means.
inline int effective_priority(const std::vector<Task>& tasks, const std::vector<int>& holder, int index,
                              bool inheritance, int depth = 0) {
  (void)holder;
  (void)inheritance;
  (void)depth;
  return tasks[static_cast<std::size_t>(index)].priority;
}

// ---- The simulator. Given; do not edit it. ----

// Runs the tasks one tick at a time. Each tick the runnable task with the highest effective priority runs; ties
// go to the earlier task. Lock and unlock take no time. A task that finds its lock held is blocked until it is
// released, and a task is done when its script is. Returns who ran each tick and when each finished.
inline Result simulate(std::vector<Task> tasks, int locks, bool inheritance) {
  std::vector<int> holder(static_cast<std::size_t>(locks), -1);
  Result result;

  auto release = [&](int lock) {
    holder[static_cast<std::size_t>(lock)] = -1;
    for (Task& other : tasks)
      if (other.blocked_on == lock) other.blocked_on = -1;
  };

  for (int tick = 0; tick < 200; ++tick) {
    bool all_done = true;
    for (const Task& task : tasks)
      if (!task.done) all_done = false;
    if (all_done) break;

    int chosen = -1;
    for (;;) {
      int best = -1;
      int best_priority = -1;
      for (std::size_t i = 0; i < tasks.size(); ++i) {
        const Task& task = tasks[i];
        if (task.done || task.arrival > tick || task.blocked_on >= 0) continue;
        const int priority = effective_priority(tasks, holder, static_cast<int>(i), inheritance);
        if (priority > best_priority) {
          best_priority = priority;
          best = static_cast<int>(i);
        }
      }
      if (best < 0) break;

      Task& task = tasks[static_cast<std::size_t>(best)];
      bool blocked = false;
      while (task.step < task.script.size() && task.script[task.step].op != Op::kCompute) {
        const Step& s = task.script[task.step];
        if (s.op == Op::kUnlock) {
          release(s.arg);
          ++task.step;
        } else if (holder[static_cast<std::size_t>(s.arg)] < 0 || holder[static_cast<std::size_t>(s.arg)] == best) {
          holder[static_cast<std::size_t>(s.arg)] = best;
          ++task.step;
        } else {
          task.blocked_on = s.arg;
          blocked = true;
          break;
        }
      }
      if (blocked) continue;
      if (task.step >= task.script.size()) {
        task.done = true;
        task.finished_at = tick - 1;
        continue;
      }
      chosen = best;
      break;
    }

    if (chosen < 0) {
      result.trace += '.';
      continue;
    }

    Task& task = tasks[static_cast<std::size_t>(chosen)];
    if (task.remaining == 0) task.remaining = task.script[task.step].arg;
    --task.remaining;
    result.trace += task.name;

    if (task.remaining == 0) {
      ++task.step;
      while (task.step < task.script.size() && task.script[task.step].op == Op::kUnlock) {
        release(task.script[task.step].arg);
        ++task.step;
      }
      if (task.step >= task.script.size()) {
        task.done = true;
        task.finished_at = tick;
      }
    }
  }

  for (const Task& task : tasks) result.finished.push_back(task.finished_at);
  return result;
}

#endif  // LESSON_SOLUTION_HPP
