// rc/capstone/teleop.hpp
//
// A robot driven from a station over a bad link, from lesson 21-06, graduated.
//
// max_timeout is the longest the stop path may wait for a heartbeat for the robot to stop within a distance (speed times timeout plus the braking distance); chance_all_lost is loss to the power of the heartbeats in a timeout; should_reset says when a person may press reset (a good link for long enough). run_remote runs a robot driven from a station on its own clock with a StopPath of 19-02: packets that are lost, delayed or cut by an outage, a latched stop that needs a reset, and counts of the stops, the time stopped and the distance travelled after an outage began.
//
// At 1 m/s the robot went 0.398, 0.598 and 1.298 m after the link went with timeouts of 0.1, 0.3 and 1.0 s (1.463, 1.863 and 3.263 m at 2 m/s); a link delay of 100, 300 and 1000 ms added 0.100, 0.290 and 1.000 m. In an hour at 5 percent loss a timeout of 0.1 s caused 185 false stops and 0.2 s none; at 20 percent 1198 (stopped for 52.1 percent of the hour) at 0.1 s, 101 at 0.2 s, 3 at 0.3 s and none at 0.5 s. A timeout of 0.667 s for 1 m at 1 m/s went 0.958 m; at 2 m/s no timeout fits.

#ifndef RC_CAPSTONE_TELEOP_HPP
#define RC_CAPSTONE_TELEOP_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <rc/control/limits.hpp>
#include <rc/safety/stop_path.hpp>
#include <rc/safety/stopping.hpp>
#include <vector>

namespace rc {
namespace capstone {

// The longest the stop path may wait for a heartbeat if the robot, going at `speed` (m/s), is to stop within `distance` (m) of the heartbeat being lost, braking at `decel` (m/s^2): the robot goes on at its speed for the timeout and then brakes, so
// distance = speed * timeout + speed^2 / (2 decel). Zero if the braking alone is more than the distance, or the speed or the deceleration is not above zero.
inline double max_timeout(double distance, double speed, double decel) {
  if (speed <= 0.0 || decel <= 0.0) return 0.0;
  const double t = (distance - speed * speed / (2.0 * decel)) / speed;
  return t > 0.0 ? t : 0.0;
}

// The chance, if each heartbeat is lost independently with probability `loss`, that a given run of `needed` heartbeats in a row is all lost: loss to the power needed. needed of 0 or below is 1 (nothing has to be lost); a loss outside 0 to 1 is brought into it.
inline double chance_all_lost(double loss, int needed) {
  const double p = std::max(0.0, std::min(1.0, loss));
  if (needed <= 0) return 1.0;
  return std::pow(p, needed);
}

// A noise source that is the same on every machine.
struct LinkNoise {
  std::uint64_t s = 424242424242ULL;
  double uniform() {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return static_cast<double>(s >> 11) / 9007199254740992.0;
  }
};

// Should the operator press reset now? Only if a reset has been asked for at all (reset_after of 0 or more), the link has been carrying packets since `good_since` (not negative), and that has been for at least reset_after seconds by `now`.
inline bool should_reset(double good_since, double now, double reset_after) { return reset_after >= 0.0 && good_since >= 0.0 && now - good_since >= reset_after; }

struct RemoteConfig {
  double speed = 1.0;               // the speed the operator asks for, m/s
  double decel = 1.5;               // how hard the robot brakes, m/s^2
  double timeout = 0.3;             // the stop path's timeout, seconds
  double heartbeat_period = 0.05;   // the operator sends a command every 50 ms
  double loss = 0.0;                // each packet is lost with this probability
  int delay_frames = 0;             // packets arrive this many 10 ms ticks late
  double outage_from = -1.0;        // the link carries nothing from this time (negative: never)
  double outage_to = -1.0;          // until this time (negative: for ever)
  double reset_after = 1.0;         // seconds of good link after a stop before the operator presses reset (negative: never)
};

struct RemoteResult {
  int stops_in_outage = 0;          // stops that began while the link was out
  int stops_otherwise = 0;          // stops that began with the link up (false stops)
  double travel_after_outage = 0.0; // how far the robot went from the start of the outage until it was still (0 if there was no outage or it never stopped)
  double time_to_stop = 0.0;        // from the start of the outage
  double distance = 0.0;
  double time_stopped = 0.0;        // seconds in which the robot was stopped, the time before the first reset included
  bool moving_at_end = false;
  int resets = 0;
  int packets_sent = 0;
  int packets_lost = 0;
};

// Run a robot driven from a station for `seconds` in ticks of 10 ms on the robot's own clock, starting stopped (as the stop path does) and with its e-stop loop closed throughout. The operator sends a command (the speed) every heartbeat_period; a packet is lost with probability `loss`, or if the link is out; one that is not lost arrives
// delay_frames ticks later, and on arrival the robot notes the heartbeat on its StopPath and the speed asked for. Every tick, in this order: send, receive, work out whether the link is good (a packet within the timeout; `good_since` is when it became so), ask the stop path for what the robot may do,
// count a stop that has just begun (as in the outage if the link was out at that time, otherwise a false stop; the first stop at the start does not count), press reset if should_reset says so, and move the speed towards what is allowed by at most decel.
inline RemoteResult run_remote(const RemoteConfig& cfg, double seconds) {
  RemoteResult r;
  const double dt = 0.01;
  rc::safety::StopPath stop(cfg.timeout);
  stop.estop_line(true);
  LinkNoise noise;
  struct Packet {
    int arrive_tick;
    double cmd;
  };
  std::deque<Packet> queue;
  double last_cmd = 0.0;
  double heard_at = -1e9;
  double good_since = -1.0;
  double v = 0.0;
  double next_send = 0.0;
  bool prev_stopped = true;
  double outage_start_distance = 0.0;
  bool outage_started = false;
  bool outage_recorded = false;
  const int steps = static_cast<int>(seconds / dt);
  for (int i = 0; i < steps; ++i) {
    const double t = i * dt;
    const bool outage = cfg.outage_from >= 0.0 && t >= cfg.outage_from && (cfg.outage_to < 0.0 || t < cfg.outage_to);
    if (cfg.outage_from >= 0.0 && t >= cfg.outage_from && !outage_started) {
      outage_started = true;
      outage_start_distance = r.distance;
    }
    if (t >= next_send) {
      next_send += cfg.heartbeat_period;
      ++r.packets_sent;
      const bool lost = outage || noise.uniform() < cfg.loss;
      if (lost) ++r.packets_lost;
      else queue.push_back({i + cfg.delay_frames, cfg.speed});
    }
    while (!queue.empty() && queue.front().arrive_tick <= i) {
      last_cmd = queue.front().cmd;
      queue.pop_front();
      stop.heartbeat(t);
      heard_at = t;
      if (good_since < 0.0) good_since = t;
    }
    if (t - heard_at > cfg.timeout) good_since = -1.0;
    double allowed = stop.output(last_cmd, t);
    bool stopped = stop.stopped();
    if (stopped && !prev_stopped) {
      if (outage) ++r.stops_in_outage;
      else ++r.stops_otherwise;
    }
    prev_stopped = stopped;
    if (stopped && should_reset(good_since, t, cfg.reset_after) && stop.reset(t)) {
      ++r.resets;
      allowed = stop.output(last_cmd, t);
      stopped = stop.stopped();
      prev_stopped = stopped;
    }
    v = rc::control::rate_limit(v, allowed, cfg.decel * dt);
    r.distance += v * dt;
    if (stopped) r.time_stopped += dt;
    if (outage_started && !outage_recorded && stopped && v <= 1e-9) {
      outage_recorded = true;
      r.time_to_stop = t - cfg.outage_from;
      r.travel_after_outage = r.distance - outage_start_distance;
    }
  }
  r.moving_at_end = v > 0.01;
  return r;
}

}  // namespace capstone
}  // namespace rc

#endif  // RC_CAPSTONE_TELEOP_HPP
