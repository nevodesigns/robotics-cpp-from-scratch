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
  // TODO: Plan, with one Run per task (id copied, started -1) and a list of the tasks in the order to consider them: by ready_at, ties in the order given.
  // For each tick t from 0 to limit, until every task has finished:
  //  1. every started task whose finish time is t or earlier is done: its thread is free again (and so is its group).
  //  2. go through the tasks in the order to consider them. Skip any already started or not yet ready (ready_at > t). Stop when no thread is free.
  //     A task in an EXCLUSIVE group (the vector says so; a group with no entry is exclusive) cannot start while another task of that group is running.
  //     Otherwise it starts: use a thread, started = t, finished = t + duration (at least 1), waited = t - ready_at.
  //     A task that cannot start does not stop the ones behind it.
  (void)tasks;
  (void)group_exclusive;
  (void)threads;
  (void)limit;
  return std::vector<Run>(tasks.size());
}

// The longest any callback of `group` waited to start. Zero if the group has none. A callback that never started counts as having waited for the whole of `limit`.
inline int worst_wait(const std::vector<Task>& tasks, const std::vector<Run>& runs, int group, int limit) {
  // TODO: the largest `waited` among the runs of tasks in this group; a task that never started counts as limit - ready_at; 0 if none
  (void)tasks;
  (void)runs;
  (void)group;
  (void)limit;
  return 0;
}

#endif  // LESSON_SOLUTION_HPP
