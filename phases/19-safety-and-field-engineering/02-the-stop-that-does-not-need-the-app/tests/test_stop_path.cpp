#include <rc/test/rc_test.hpp>

#include <iostream>

#include "solution.hpp"

namespace {

// What the usual first attempt does: it looks at the age of the last message only when a message arrives. With no messages nothing ever looks.
struct NaiveGuard {
  double timeout = 0.3;
  double last_beat = 0.0;
  double last_command = 0.0;
  void on_message(double command, double now) {
    if (now - last_beat <= timeout) last_command = command;
    else last_command = 0.0;
    last_beat = now;
  }
  double output() const { return last_command; }
};

}  // namespace

RC_TEST("it starts stopped, and a loop nobody has reported on is an open loop") {
  StopPath p(0.3);
  RC_CHECK(p.stopped());
  RC_CHECK(p.reason() == StopReason::startup);
  RC_CHECK(p.output(1.0, 0.0) == 0.0);
  p.heartbeat(0.0);
  RC_CHECK(p.output(1.0, 0.1) == 0.0);              // a heartbeat alone does not start it
  RC_CHECK(p.reason() == StopReason::estop);        // and the loop was never reported closed: open
  std::cout << "  stopped at power up; with heartbeats but no word from the e-stop loop it read as open: reason " << describe(p.reason()) << "\n\n";
}

RC_TEST("a reset needs a closed loop and a fresh heartbeat, and a refused reset changes nothing") {
  StopPath p(0.3);
  RC_CHECK(!p.reset(0.0));                           // nothing heard
  p.estop_line(true);
  RC_CHECK(!p.reset(0.0));                           // loop closed, no heartbeat
  p.heartbeat(1.0);
  RC_CHECK(p.reset(1.1));                            // both
  RC_CHECK(!p.stopped());
  RC_CHECK(p.output(0.7, 1.1) == 0.7);
  p.estop_line(false);
  RC_CHECK(p.output(0.7, 1.15) == 0.0);
  p.heartbeat(1.2);
  RC_CHECK(!p.reset(1.2));                           // loop open, heartbeat fresh: refused
  RC_CHECK(p.stopped());
  RC_CHECK(p.reason() == StopReason::estop);
  p.estop_line(true);
  RC_CHECK(p.stopped());                             // closing the loop is not a restart
  RC_CHECK(!p.reset(2.0));                           // the heartbeat is stale by now
  p.heartbeat(2.0);
  RC_CHECK(p.reset(2.05));
  RC_CHECK(p.output(0.4, 2.05) == 0.4);
}

RC_TEST("when the heartbeat stops, the robot's own loop stops it one timeout later, with nobody sending a thing") {
  StopPath p(0.3);
  p.estop_line(true);
  p.heartbeat(0.0);
  RC_CHECK(p.reset(0.0));
  double stopped_at = -1.0;
  double last_beat = 0.0;
  // The application sends a heartbeat every 0.1 s, and dies at 1.0 s. The robot's loop runs every 10 ms and keeps asking for the last command it was given.
  for (int i = 0; i <= 300; ++i) {
    const double t = i * 0.01;
    if (t <= 1.0 + 1e-9 && i % 10 == 0) {
      p.heartbeat(t);
      last_beat = t;
    }
    const double out = p.output(0.8, t);
    if (out == 0.0 && stopped_at < 0.0) stopped_at = t;
  }
  std::cout << "  last heartbeat at " << last_beat << " s, timeout 0.3 s: the robot stopped at " << stopped_at << " s, " << (stopped_at - last_beat) << " s later\n";
  RC_CHECK(stopped_at > 1.29);
  RC_CHECK(stopped_at < 1.32);
  RC_CHECK(p.reason() == StopReason::link_lost);
  p.heartbeat(2.5);                                  // the link comes back
  RC_CHECK(p.output(0.8, 2.51) == 0.0);              // and the robot stays stopped
  std::cout << "  the link came back at 2.5 s and the output was still " << p.output(0.8, 2.52) << ": it latched\n\n";
  RC_CHECK(p.reset(2.52));
  RC_CHECK(p.output(0.8, 2.53) == 0.8);
}

RC_TEST("a guard that only looks when a message arrives never stops when the messages stop") {
  NaiveGuard g;
  g.on_message(0.8, 0.0);
  g.on_message(0.8, 0.1);
  g.on_message(0.8, 0.2);                             // the application dies here
  double out_at_5s = g.output();
  std::cout << "  messages stopped at 0.2 s; the naive guard's output at 5 s was " << out_at_5s << " (the last command, still driving)\n";
  RC_CHECK(out_at_5s == 0.8);
  StopPath p(0.3);
  p.estop_line(true);
  p.heartbeat(0.0);
  p.reset(0.0);
  p.heartbeat(0.1);
  p.heartbeat(0.2);
  const double safe = p.output(0.8, 5.0);
  std::cout << "  the stop path, called by the robot's loop, gave " << safe << " at 5 s\n\n";
  RC_CHECK(safe == 0.0);
}

RC_TEST("an open loop stops at the next tick and outranks a link that was already lost") {
  StopPath p(0.3);
  p.estop_line(true);
  p.heartbeat(0.0);
  p.reset(0.0);
  p.output(0.5, 1.0);                                 // stale: link lost
  RC_CHECK(p.reason() == StopReason::link_lost);
  p.estop_line(false);
  p.output(0.5, 1.01);
  RC_CHECK(p.reason() == StopReason::estop);          // upgraded
  p.heartbeat(1.02);
  p.estop_line(true);
  RC_CHECK(p.reason() == StopReason::estop);          // closing the loop does not clear it
  RC_CHECK(p.reset(1.03));
  p.estop_line(false);
  const double out = p.output(0.9, 1.04);
  RC_CHECK(out == 0.0);                               // one tick, no timeout
  std::cout << "  a wire cut at 1.04 s stopped the output on the very next call: " << out << "\n\n";
}

RC_TEST("a clock that goes backwards is a stale heartbeat, not an immortal one") {
  StopPath p(0.3);
  p.estop_line(true);
  p.heartbeat(100.0);
  RC_CHECK(p.reset(100.0));
  RC_CHECK(p.output(0.6, 100.1) == 0.6);
  const double out = p.output(0.6, 50.0);             // the time was set back
  std::cout << "  heartbeat at 100 s, then the clock read 50 s: output " << out << ", stale " << (p.stale(50.0) ? "yes" : "no") << "\n\n";
  RC_CHECK(p.stale(50.0));
  RC_CHECK(out == 0.0);
}
