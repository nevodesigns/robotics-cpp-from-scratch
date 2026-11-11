// rc/bus/action.hpp
//
// A goal that takes time, from lesson 17-10, graduated.
//
// A goal moves through accepted, executing, and then succeeded, aborted, or canceling and canceled, in the legal order only, and a finished goal never changes. A cancel puts a goal in canceling and only the
// server's confirmation makes it canceled, so a cancel that races the finish still ends succeeded. Progress is accepted only while a goal executes: after that it is dropped and counted. A server decides what a
// second goal does while one runs: reject_new turns it away (-1), preempt_old asks the first to cancel and accepts the second.
//
// Succeeded, canceled and aborted goals, then every call made on each: 0 changed. Feedback to a goal that was canceled kept 2 and dropped 3. Under reject_new the second goal got -1 and, after the first finished,
// the third got 1; under preempt_old the first was canceling and the second accepted.

#ifndef RC_BUS_ACTION_HPP
#define RC_BUS_ACTION_HPP

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace rc {
namespace bus {

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
    if (state_ == GoalState::accepted || state_ == GoalState::executing) {
      state_ = GoalState::canceling;
      return true;
    }
    return false;
  }
  bool succeed() {
    if (state_ == GoalState::executing || state_ == GoalState::canceling) {
      state_ = GoalState::succeeded;
      return true;
    }
    return false;
  }
  bool abort() {
    if (state_ == GoalState::executing || state_ == GoalState::canceling) {
      state_ = GoalState::aborted;
      return true;
    }
    return false;
  }
  bool confirm_canceled() { return move(GoalState::canceling, GoalState::canceled); }

  // Progress to report. Only a goal that is executing has any; after that it is dropped and counted.
  bool feedback(double progress) {
    if (state_ != GoalState::executing) {
      ++dropped_feedback_;
      return false;
    }
    feedback_.push_back(progress);
    return true;
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
    if (active_ >= 0 && !is_terminal(goals_[active_]->state())) {
      if (policy_ == WhenBusy::reject_new) {
        ++rejected_;
        return -1;
      }
      goals_[active_]->request_cancel();
      ++preempted_;
    }
    const int id = static_cast<int>(goals_.size());
    goals_.push_back(std::make_shared<GoalHandle>());
    active_ = id;
    return id;
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

}  // namespace bus
}  // namespace rc

#endif  // RC_BUS_ACTION_HPP
