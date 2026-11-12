#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <rc/safety/cross_check.hpp>
#include <rc/safety/geofence.hpp>
#include <rc/safety/power.hpp>
#include <rc/safety/tilt.hpp>
#include <string>

// What each monitor of this phase says, in one place. A default Inputs is the state in which nothing is known, and it is the state in which everything is wrong: the supervisor must stop on it.
struct Inputs {
  bool estop_closed = false;
  bool heartbeat_fresh = false;
  rc::safety::FenceState fence = rc::safety::FenceState::unknown;
  rc::safety::TiltState tilt = rc::safety::TiltState::stop;
  rc::safety::CheckState sensors = rc::safety::CheckState::faulted;
  rc::safety::PowerLimit power;     // a default one is a stop
  double range_cap = 0.0;           // the speed the range allows (lesson 19-05), m/s
};

struct SupervisorConfig {
  double top_speed = 2.0;
  double slow_speed = 0.5;          // the speed under a warning
};

enum class Mode { normal, degraded, stopped };

// The reasons to stop, as bits.
enum StopCause : unsigned { cause_estop = 1u, cause_link = 2u, cause_fence = 4u, cause_tilt = 8u, cause_sensors = 16u, cause_power = 32u };
// The reasons to go slower, as bits.
enum LimitCause : unsigned { limit_fence = 1u, limit_tilt = 2u, limit_sensors = 4u, limit_power = 8u, limit_range = 16u };

struct Decision {
  Mode mode = Mode::stopped;
  double speed_cap = 0.0;
  unsigned stop_causes = 0;
  unsigned limit_causes = 0;
};

// The reasons to stop right now: the e-stop loop open, the heartbeat stale, the robot outside the fence or its position unknown, a tilt stop, a faulted sensor pair, and the power monitor's stop.
inline unsigned stop_causes(const Inputs& in) {
  // TODO: cause_estop if the loop is not closed; cause_link if the heartbeat is not fresh; cause_fence if the fence is outside or unknown; cause_tilt on a tilt stop; cause_sensors on a faulted pair; cause_power if the power state is stop. Return the bits
  (void)in;
  return 0u;
}

// The reasons to go slower: a fence or tilt warning, a suspect sensor pair, a derated power share, and a range that allows less than the top speed.
inline unsigned limit_causes(const Inputs& in, const SupervisorConfig& cfg) {
  // TODO: limit_fence on a fence warning; limit_tilt on a tilt warn; limit_sensors on a suspect pair; limit_power when the power state is derate; limit_range if range_cap is NaN or below cfg.top_speed
  (void)in;
  (void)cfg;
  return 0u;
}

// The speed cap when nothing says stop: the top speed, cut to the slow speed by a fence, tilt or sensor warning, to the power share times the top speed when derated, and to the range's cap (zero if that is not a number or below zero).
inline double speed_cap(const Inputs& in, const SupervisorConfig& cfg) {
  // TODO: cap = top_speed. If limit_fence, limit_tilt or limit_sensors: min with slow_speed. If limit_power: min with top_speed * power.factor. If limit_range: min with range_cap (0 if NaN, never below 0)
  (void)in;
  (void)cfg;
  return 0.0;
}

class Supervisor {
 public:
  explicit Supervisor(const SupervisorConfig& c) : cfg_(c) {}

  // One step. Any reason to stop latches the stop, and every reason seen since the latch is kept (so that when the robot has been stopped by one thing and a second turns up, both are on record).
  // While latched the answer is stopped, with a cap of zero, whatever the inputs now say. Otherwise normal (the cap is the top speed) or degraded (anything lower).
  Decision step(const Inputs& in) {
    // TODO: now = stop_causes(in). If it is not zero: latched_ = true and seen_ |= now. If latched_: a Decision of mode stopped, cap 0, stop_causes seen_, no limit causes.
    // Otherwise limit_causes and speed_cap, and the mode is degraded if the cap is below top_speed, normal if not
    (void)in;
    (void)cfg_;
    (void)latched_;
    (void)seen_;
    return Decision();
  }

  // Let the robot go again, if nothing now says stop. Refused (false, nothing changes) otherwise.
  bool reset(const Inputs& in) {
    // TODO: refuse (false, nothing changed) if stop_causes(in) is not zero; otherwise latched_ = false, seen_ = 0, true
    (void)in;
    (void)latched_;
    (void)seen_;
    return false;
  }

  bool latched() const { return latched_; }

 private:
  SupervisorConfig cfg_;
  bool latched_ = true;       // it starts stopped
  unsigned seen_ = 0;
};

// The causes as words, for a log: "estop, link".
inline std::string causes_text(unsigned c) {
  const struct { unsigned bit; const char* name; } names[] = {{cause_estop, "estop"}, {cause_link, "link"}, {cause_fence, "fence"}, {cause_tilt, "tilt"}, {cause_sensors, "sensors"}, {cause_power, "power"}};
  std::string s;
  for (const auto& n : names)
    if (c & n.bit) s += (s.empty() ? "" : ", ") + std::string(n.name);
  return s.empty() ? "none" : s;
}

#endif  // LESSON_SOLUTION_HPP
