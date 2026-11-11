#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Where a goal is in its life.
enum class GoalState { accepted, executing, canceling, succeeded, canceled, aborted };

inline bool is_terminal(GoalState s) { return s == GoalState::succeeded || s == GoalState::canceled || s == GoalState::aborted; }

// One goal, such as "move the arm to this pose". It moves through the states in one legal order and cannot be put anywhere else.
//   accepted  -> executing, or canceling (cancelled before it began)
//   executing -> canceling, succeeded or aborted
//   canceling -> canceled, succeeded (the work finished before the cancel could take effect) or aborted
// A finished goal (succeeded, canceled or aborted) never changes again. Each call says whether it was a legal move.
class GoalHandle {
 public:
  GoalState state() const { return state_; }

  bool execute() { return move(GoalState::accepted, GoalState::executing); }
  bool request_cancel() {
    // TODO: a goal that is accepted or executing becomes canceling and the answer is true; any other state is not changed and the answer is false
    return false;
  }
  bool succeed() {
    // TODO: executing or canceling becomes succeeded (true); anything else is unchanged (false)
    return false;
  }
  bool abort() {
    // TODO: executing or canceling becomes aborted (true); anything else is unchanged (false)
    return false;
  }
  bool confirm_canceled() { return move(GoalState::canceling, GoalState::canceled); }

  // Progress to report. Only a goal that is executing has any; after that it is dropped and counted.
  bool feedback(double progress) {
    // TODO: only an executing goal takes feedback: keep progress in feedback_ and return true.
    // In any other state add one to dropped_feedback_ and return false.
    (void)progress;
    (void)feedback_;
    (void)dropped_feedback_;
    return false;
  }

  const std::vector<double>& feedback_log() const { return feedback_; }
  int dropped_feedback() const { return dropped_feedback_; }

 private:
  bool move(GoalState from, GoalState to) {
    if (state_ != from) return false;
    state_ = to;
    return true;
  }
  GoalState state_ = GoalState::accepted;
  std::vector<double> feedback_;
  int dropped_feedback_ = 0;
};

// What to do with a new goal when one is already running.
enum class WhenBusy { reject_new, preempt_old };

// Hands out goals. At most one is active at a time.
class ActionServer {
 public:
  explicit ActionServer(WhenBusy policy) : policy_(policy) {}

  // A new goal. Returns its id, or -1 if it was refused. With `preempt_old` the goal that is running is asked to cancel and the new one is
  // accepted at once.
  int send_goal() {
    // TODO: If there is an active goal (active_ >= 0) that has not finished (is_terminal is false):
    //   with reject_new add one to rejected_ and return -1;
    //   with preempt_old ask the active goal to cancel (request_cancel) and add one to preempted_.
    // Then make a new GoalHandle (std::make_shared), add it to goals_, make it the active one and return its id (its index).
    (void)policy_;
    (void)goals_;
    (void)active_;
    (void)rejected_;
    (void)preempted_;
    return -1;
  }

  GoalHandle* goal(int id) {
    if (id < 0 || id >= static_cast<int>(goals_.size())) return nullptr;
    return goals_[static_cast<std::size_t>(id)].get();
  }

  int rejected() const { return rejected_; }
  int preempted() const { return preempted_; }

 private:
  WhenBusy policy_;
  std::vector<std::shared_ptr<GoalHandle>> goals_;
  int active_ = -1;
  int rejected_ = 0;
  int preempted_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
