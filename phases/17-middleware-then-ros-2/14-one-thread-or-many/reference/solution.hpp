#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <vector>

// A callback waiting to run: when it became ready, how long it takes, and which group it belongs to.
struct Task {
  int id = 0;
  int ready_at = 0;
  int duration = 1;
  int group = 0;
};

// What happened to one task.
struct Run {
  int id = 0;
  int started = -1;      // -1 if it never got to run
  int finished = -1;
  int waited = 0;        // started - ready_at
};

// An executor with `threads` threads, run in whole ticks. A group is either exclusive (its callbacks never overlap, however many threads there are)
// or not. At each tick, callbacks that have finished free their thread and their group, and then the ready callbacks are looked at in the order
// they became ready, ties in the order given: each starts if a thread is free and its group allows it. A callback that cannot start does not stop
// those behind it. `group_exclusive[g]` is the rule for group g; a group with no entry is exclusive. Gives up after `limit` ticks.
inline std::vector<Run> simulate(const std::vector<Task>& tasks, const std::vector<bool>& group_exclusive, int threads, int limit) {
  const std::size_t n = tasks.size();
  std::vector<Run> runs(n);
  std::vector<bool> started(n, false);
  std::vector<bool> done(n, false);
  for (std::size_t i = 0; i < n; ++i) runs[i].id = tasks[i].id;

  // The order to consider tasks in: by when they became ready, then as given.
  std::vector<std::size_t> order(n);
  for (std::size_t i = 0; i < n; ++i) order[i] = i;
  for (std::size_t i = 1; i < n; ++i) {
    const std::size_t x = order[i];
    std::size_t j = i;
    while (j > 0 && tasks[order[j - 1]].ready_at > tasks[x].ready_at) {
      order[j] = order[j - 1];
      --j;
    }
    order[j] = x;
  }

  int free_threads = threads < 1 ? 1 : threads;
  std::size_t finished_count = 0;
  for (int t = 0; t <= limit && finished_count < n; ++t) {
    for (std::size_t i = 0; i < n; ++i)
      if (started[i] && !done[i] && runs[i].finished <= t) {
        done[i] = true;
        ++free_threads;
        ++finished_count;
      }
    for (std::size_t k = 0; k < n && free_threads > 0; ++k) {
      const std::size_t i = order[k];
      if (started[i] || tasks[i].ready_at > t) continue;
      const int g = tasks[i].group;
      const bool exclusive = g < 0 || g >= static_cast<int>(group_exclusive.size()) ? true : group_exclusive[static_cast<std::size_t>(g)];
      bool blocked = false;
      if (exclusive)
        for (std::size_t j = 0; j < n; ++j)
          if (started[j] && !done[j] && tasks[j].group == g) blocked = true;
      if (blocked) continue;
      started[i] = true;
      --free_threads;
      runs[i].started = t;
      runs[i].finished = t + (tasks[i].duration < 1 ? 1 : tasks[i].duration);
      runs[i].waited = t - tasks[i].ready_at;
    }
  }
  return runs;
}

// The longest any callback of `group` waited to start. Zero if the group has none. A callback that never started counts as having waited for the whole of `limit`.
inline int worst_wait(const std::vector<Task>& tasks, const std::vector<Run>& runs, int group, int limit) {
  int worst = 0;
  for (std::size_t i = 0; i < tasks.size() && i < runs.size(); ++i) {
    if (tasks[i].group != group) continue;
    const int w = runs[i].started < 0 ? limit - tasks[i].ready_at : runs[i].waited;
    if (w > worst) worst = w;
  }
  return worst;
}

#endif  // LESSON_SOLUTION_HPP
