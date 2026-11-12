#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

enum class StopReason { none, startup, link_lost, estop };

inline const char* describe(StopReason r) {
  switch (r) {
    case StopReason::none: return "none";
    case StopReason::startup: return "startup";
    case StopReason::link_lost: return "link lost";
    case StopReason::estop: return "e-stop";
  }
  return "?";
}

// The stop path that runs on the robot, whatever happens to the application and the network. It is driven by the robot's own clock: output() is called by the robot's own control loop every tick, with the time on the robot,
// and that is the only thing that makes it work. Nothing in it waits for a message.
//
// It starts stopped. It latches: once stopped it stays stopped until reset() is called, and reset() is refused unless the e-stop loop is closed and a heartbeat has been heard within the timeout.
class StopPath {
 public:
  explicit StopPath(double timeout) : timeout_(timeout) {}

  // A heartbeat from the operator station (or the application). It is the only thing that keeps the robot allowed to move, and it is never enough to start it again by itself.
  void heartbeat(double now) {
    last_beat_ = now;
    beat_seen_ = true;
  }

  // The e-stop loop is normally closed: closed is healthy, open is stop. A button pressed, a wire cut and a connector pulled out all read as open, and so does a loop nobody has reported on yet.
  void estop_line(bool closed) { line_closed_ = closed; }

  // True if no heartbeat has been heard, or the clock has gone backwards since the last, or the last was more than the timeout ago.
  bool stale(double now) const { return !beat_seen_ || now < last_beat_ || (now - last_beat_) > timeout_; }

  // Look at the inputs and latch a stop if there is a reason to. An open loop is the strongest reason and replaces a weaker one already latched; a stale heartbeat latches only if the robot is not already stopped.
  void tick(double now) {
    if (!line_closed_) {
      stopped_ = true;
      reason_ = StopReason::estop;
    } else if (stale(now) && !stopped_) {
      stopped_ = true;
      reason_ = StopReason::link_lost;
    }
  }

  // Let the robot move again. Refused (false) unless the loop is closed and a heartbeat is fresh; nothing changes if it is refused.
  bool reset(double now) {
    if (!line_closed_) return false;
    if (stale(now)) return false;
    stopped_ = false;
    reason_ = StopReason::none;
    return true;
  }

  // What to send to the motors: the command if the robot is allowed to move, zero if it is not. This is the function the robot's own loop calls every tick.
  double output(double command, double now) {
    tick(now);
    return stopped_ ? 0.0 : command;
  }

  bool stopped() const { return stopped_; }
  StopReason reason() const { return reason_; }

 private:
  double timeout_ = 0.3;
  double last_beat_ = 0.0;
  bool beat_seen_ = false;
  bool line_closed_ = false;
  bool stopped_ = true;
  StopReason reason_ = StopReason::startup;
};

#endif  // LESSON_SOLUTION_HPP
