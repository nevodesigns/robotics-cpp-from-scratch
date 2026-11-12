#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <rc/control/limits.hpp>
#include <rc/safety/stop_path.hpp>
#include <rc/safety/stopping.hpp>
#include <vector>

// The longest the stop path may wait for a heartbeat if the robot, going at `speed` (m/s), is to stop within `distance` (m) of the heartbeat being lost, braking at `decel` (m/s^2): the robot goes on at its speed for the timeout and then brakes, so
// distance = speed * timeout + speed^2 / (2 decel). Zero if the braking alone is more than the distance, or the speed or the deceleration is not above zero.
inline double max_timeout(double distance, double speed, double decel) {
  // TODO: zero if speed or decel is not above zero; t = (distance - speed^2 / (2 decel)) / speed; zero if t is not above zero
  (void)distance;
  (void)speed;
  (void)decel;
  return 0.0;
}

// The chance, if each heartbeat is lost independently with probability `loss`, that a given run of `needed` heartbeats in a row is all lost: loss to the power needed. needed of 0 or below is 1 (nothing has to be lost); a loss outside 0 to 1 is brought into it.
inline double chance_all_lost(double loss, int needed) {
  // TODO: loss brought into 0 to 1; 1 if needed is 0 or below; otherwise loss to the power needed (std::pow)
  (void)loss;
  (void)needed;
  return 1.0;
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
inline bool should_reset(double good_since, double now, double reset_after) {
  // TODO: reset_after at least 0, good_since not negative, and now - good_since at least reset_after
  (void)good_since;
  (void)now;
  (void)reset_after;
  return false;
}

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

#endif  // LESSON_SOLUTION_HPP
