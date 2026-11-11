#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

const char* name_of(GoalState s) {
  switch (s) {
    case GoalState::accepted: return "accepted";
    case GoalState::executing: return "executing";
    case GoalState::canceling: return "canceling";
    case GoalState::succeeded: return "succeeded";
    case GoalState::canceled: return "canceled";
    case GoalState::aborted: return "aborted";
  }
  return "?";
}

}  // namespace

RC_TEST("a goal that goes well passes through accepted, executing and succeeded, in that order") {
  GoalHandle g;
  RC_CHECK(g.state() == GoalState::accepted);
  RC_CHECK(g.execute());
  RC_CHECK(g.state() == GoalState::executing);
  RC_CHECK(g.succeed());
  RC_CHECK(g.state() == GoalState::succeeded);
  RC_CHECK(is_terminal(g.state()));
  RC_CHECK(!is_terminal(GoalState::accepted));
  RC_CHECK(!is_terminal(GoalState::executing));
  RC_CHECK(!is_terminal(GoalState::canceling));
}

RC_TEST("a cancel moves a running goal to canceling first and only the server's confirmation makes it canceled") {
  GoalHandle g;
  g.execute();
  RC_CHECK(g.request_cancel());
  RC_CHECK(g.state() == GoalState::canceling);
  RC_CHECK(!is_terminal(g.state()));
  RC_CHECK(g.confirm_canceled());
  RC_CHECK(g.state() == GoalState::canceled);

  GoalHandle early;
  RC_CHECK(early.request_cancel());                      // cancelled before it ever began
  RC_CHECK(early.state() == GoalState::canceling);
  RC_CHECK(!early.execute());                            // and it cannot begin now
  RC_CHECK(early.confirm_canceled());

  GoalHandle not_running;
  RC_CHECK(!not_running.confirm_canceled());             // nothing was asked to cancel
}

RC_TEST("a cancel that comes too late does not undo a goal that has finished, and one that races the finish still ends as succeeded") {
  GoalHandle done;
  done.execute();
  done.succeed();
  RC_CHECK(!done.request_cancel());
  RC_CHECK(done.state() == GoalState::succeeded);

  GoalHandle race;
  race.execute();
  race.request_cancel();                                 // the cancel is on its way...
  RC_CHECK(race.succeed());                              // ...and the arm arrives first
  std::cout << "  cancel requested, then the work finished before the cancel took effect: " << name_of(race.state()) << "\n\n";
  RC_CHECK(race.state() == GoalState::succeeded);
  RC_CHECK(!race.confirm_canceled());
}

RC_TEST("a finished goal never changes again, whatever is asked of it") {
  int changed = 0;
  for (int how = 0; how < 3; ++how) {
    GoalHandle g;
    g.execute();
    if (how == 0) g.succeed();
    if (how == 1) {
      g.request_cancel();
      g.confirm_canceled();
    }
    if (how == 2) g.abort();
    const GoalState before = g.state();
    g.execute();
    g.request_cancel();
    g.succeed();
    g.abort();
    g.confirm_canceled();
    if (g.state() != before) ++changed;
    RC_CHECK(is_terminal(g.state()));
  }
  std::cout << "  succeeded, canceled and aborted goals, then every call made on each: " << changed << " changed\n\n";
  RC_CHECK(changed == 0);
  GoalHandle fresh;
  RC_CHECK(!fresh.succeed());                            // cannot succeed before it executes
  RC_CHECK(!fresh.abort());
}

RC_TEST("progress is reported while a goal runs, and is dropped and counted once it is anywhere else") {
  GoalHandle g;
  RC_CHECK(!g.feedback(0.0));                            // accepted, not started
  g.execute();
  RC_CHECK(g.feedback(0.25));
  RC_CHECK(g.feedback(0.5));
  g.request_cancel();
  RC_CHECK(!g.feedback(0.6));                            // canceling
  g.confirm_canceled();
  RC_CHECK(!g.feedback(0.7));
  std::cout << "  feedback kept " << g.feedback_log().size() << ", dropped " << g.dropped_feedback() << "\n\n";
  RC_CHECK(g.feedback_log().size() == 2);
  RC_CHECK(g.feedback_log().size() == 2 && g.feedback_log()[1] == 0.5);
  RC_CHECK(g.dropped_feedback() == 3);
}

RC_TEST("a second goal while one runs is turned away or takes over, and the server's rule decides which") {
  ActionServer strict(WhenBusy::reject_new);
  const int a = strict.send_goal();
  RC_CHECK(a == 0);
  if (a != 0) return;                                    // nothing below can be done without a goal
  strict.goal(a)->execute();
  const int b = strict.send_goal();
  RC_CHECK(a == 0);
  RC_CHECK(b == -1);
  RC_CHECK(strict.rejected() == 1);
  RC_CHECK(strict.goal(a)->state() == GoalState::executing);
  strict.goal(a)->succeed();
  const int c = strict.send_goal();                      // free again
  RC_CHECK(c == 1);

  ActionServer takeover(WhenBusy::preempt_old);
  const int x = takeover.send_goal();
  RC_CHECK(x == 0);
  if (x != 0) return;
  takeover.goal(x)->execute();
  const int y = takeover.send_goal();
  RC_CHECK(y == 1);
  if (y != 1) return;
  std::cout << "  reject_new: second goal " << b << " (turned away), after the first finished the third got " << c << "\n"
            << "  preempt_old: first goal is " << name_of(takeover.goal(x)->state()) << " and the second is " << name_of(takeover.goal(y)->state()) << "\n\n";
  RC_CHECK(y == 1);
  RC_CHECK(takeover.goal(x)->state() == GoalState::canceling);
  RC_CHECK(takeover.goal(y)->state() == GoalState::accepted);
  RC_CHECK(takeover.preempted() == 1);
  RC_CHECK(takeover.goal(99) == nullptr);
  RC_CHECK(takeover.goal(-1) == nullptr);
}
