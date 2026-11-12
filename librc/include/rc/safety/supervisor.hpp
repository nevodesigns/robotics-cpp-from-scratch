// rc/safety/supervisor.hpp
//
// A safety supervisor, from lesson 19-10, graduated.
//
// Inputs holds what each monitor of the phase says (e-stop loop, heartbeat, fence, tilt, sensor pair, power, range cap) and a default Inputs is the state in which nothing is known, which is a stop. stop_causes turns it into bits (estop, link, fence, tilt, sensors, power); limit_causes and speed_cap into the reasons to go slower and the speed: the top speed cut to the slow speed by a fence, tilt or sensor warning, to the top speed times the power share when derated, and to the range's cap.
// Supervisor::step latches a stop, keeps every cause seen since the latch, and stays stopped when the causes go away; reset is refused while a cause remains. It starts stopped.
//
// Over all 1024 combinations of ten conditions, 1008 stopped, 15 degraded and 1 was normal, and none of the 10240 additions of a fault made the mode or the cap less strict. A tilt stop followed by a lost link kept both causes on record.

#ifndef RC_SAFETY_SUPERVISOR_HPP
#define RC_SAFETY_SUPERVISOR_HPP

#include <algorithm>
#include <cmath>
#include <rc/safety/cross_check.hpp>
#include <rc/safety/geofence.hpp>
#include <rc/safety/power.hpp>
#include <rc/safety/tilt.hpp>
#include <string>

namespace rc {
namespace safety {

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
  unsigned c = 0;
  if (!in.estop_closed) c |= cause_estop;
  if (!in.heartbeat_fresh) c |= cause_link;
  if (in.fence == rc::safety::FenceState::outside || in.fence == rc::safety::FenceState::unknown) c |= cause_fence;
  if (in.tilt == rc::safety::TiltState::stop) c |= cause_tilt;
  if (in.sensors == rc::safety::CheckState::faulted) c |= cause_sensors;
  if (in.power.state == rc::safety::PowerState::stop) c |= cause_power;
  return c;
}

// The reasons to go slower: a fence or tilt warning, a suspect sensor pair, a derated power share, and a range that allows less than the top speed.
inline unsigned limit_causes(const Inputs& in, const SupervisorConfig& cfg) {
  unsigned c = 0;
  if (in.fence == rc::safety::FenceState::warning) c |= limit_fence;
  if (in.tilt == rc::safety::TiltState::warn) c |= limit_tilt;
  if (in.sensors == rc::safety::CheckState::suspect) c |= limit_sensors;
  if (in.power.state == rc::safety::PowerState::derate) c |= limit_power;
  if (std::isnan(in.range_cap) || in.range_cap < cfg.top_speed) c |= limit_range;
  return c;
}

// The speed cap when nothing says stop: the top speed, cut to the slow speed by a fence, tilt or sensor warning, to the power share times the top speed when derated, and to the range's cap (zero if that is not a number or below zero).
inline double speed_cap(const Inputs& in, const SupervisorConfig& cfg) {
  double cap = cfg.top_speed;
  const unsigned l = limit_causes(in, cfg);
  if (l & (limit_fence | limit_tilt | limit_sensors)) cap = std::min(cap, cfg.slow_speed);
  if (l & limit_power) cap = std::min(cap, cfg.top_speed * in.power.factor);
  if (l & limit_range) cap = std::min(cap, std::isnan(in.range_cap) ? 0.0 : std::max(in.range_cap, 0.0));
  return cap;
}

class Supervisor {
 public:
  explicit Supervisor(const SupervisorConfig& c) : cfg_(c) {}

  // One step. Any reason to stop latches the stop, and every reason seen since the latch is kept (so that when the robot has been stopped by one thing and a second turns up, both are on record).
  // While latched the answer is stopped, with a cap of zero, whatever the inputs now say. Otherwise normal (the cap is the top speed) or degraded (anything lower).
  Decision step(const Inputs& in) {
    const unsigned now = stop_causes(in);
    if (now != 0) {
      latched_ = true;
      seen_ |= now;
    }
    Decision d;
    if (latched_) {
      d.mode = Mode::stopped;
      d.speed_cap = 0.0;
      d.stop_causes = seen_;
      d.limit_causes = 0;
      return d;
    }
    d.limit_causes = limit_causes(in, cfg_);
    d.speed_cap = speed_cap(in, cfg_);
    d.mode = d.speed_cap < cfg_.top_speed ? Mode::degraded : Mode::normal;
    return d;
  }

  // Let the robot go again, if nothing now says stop. Refused (false, nothing changes) otherwise.
  bool reset(const Inputs& in) {
    if (stop_causes(in) != 0) return false;
    latched_ = false;
    seen_ = 0;
    return true;
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

}  // namespace safety
}  // namespace rc

#endif  // RC_SAFETY_SUPERVISOR_HPP
