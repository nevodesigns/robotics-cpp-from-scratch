#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <rc/control/limits.hpp>
#include <rc/nav/filter.hpp>
#include <rc/nav/odometry.hpp>
#include <rc/safety/geofence.hpp>
#include <rc/safety/supervisor.hpp>
#include <rc/sim/diff_drive.hpp>
#include <vector>

// The robot's wheels are not quite what its software thinks: the real left and right wheels travel `left_scale` and `right_scale` times the distance asked of them, and the real wheel base is `base_scale` times the nominal one.
struct WheelError {
  double left_scale = 1.0;
  double right_scale = 1.0;
  double base_scale = 1.0;
};

// How far the believed position is from the true one.
inline double position_error(const rc::sim::Pose& believed, const rc::sim::Pose& truth) {
  // TODO: the distance between the two positions (std::hypot of the differences in x and y)
  (void)believed;
  (void)truth;
  return 0.0;
}

// The buffer of lesson 19-06 for a robot whose position is a guess: how far from the edge of the fence it must stop is its stopping distance plus the drift it may have picked up since its last fix, assumed to be `drift_per_metre` for each metre travelled since then.
// This is the same sum as rc::safety::required_buffer, and it is written out here because what is being tested is what to put in it.
inline double stop_buffer(double stopping_distance, double drift_per_metre, double metres_since_fix) {
  // TODO: the stopping distance plus the drift per metre times the metres since the last fix, neither of the last two below zero (rc::safety::required_buffer does this)
  (void)stopping_distance;
  (void)drift_per_metre;
  (void)metres_since_fix;
  return 0.0;
}

// Take a position fix: a measured x and y, each with a standard deviation `sigma`, are fused into the belief by the two filters (the correction of rc::nav::Filter1D with a measurement variance of sigma squared), and the believed position is set to the fused x and y, with the heading as it was.
inline void apply_fix(rc::nav::Filter1D& fx, rc::nav::Filter1D& fy, double zx, double zy, double sigma, rc::nav::Odometry& belief) {
  // TODO: correct each filter with its measurement and a variance of sigma * sigma; take the believed pose, put the two filters' estimated values in its x and y, and give it back to the odometry with belief.correct
  (void)fx;
  (void)fy;
  (void)zx;
  (void)zy;
  (void)sigma;
  (void)belief;
}

// A noise source that is the same on every machine: a 64 bit generator and the sum of twelve uniform numbers.
struct LocalisationNoise {
  std::uint64_t s = 1234567891011ULL;
  double uniform() {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return static_cast<double>(s >> 11) / 9007199254740992.0;
  }
  double gauss() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }
};

struct LocalisationConfig {
  double speed = 0.5;                 // m/s, straight ahead
  double beacon_period = -1.0;        // seconds between position fixes (negative: none)
  double beacon_sigma = 0.1;          // the standard deviation of a fix, metres
  double beacon_dead_at = -1.0;       // no fixes from this time on
  bool use_buffer = false;            // stop when the believed position is nearer the edge of the corridor than the buffer
  double stopping_distance = 0.5;     // for the buffer
  double assumed_drift = 0.1;         // metres of drift assumed for each metre since the last fix
  double half_width = 1.5;            // the corridor is this far either side of the x axis
  WheelError error;
};

struct LocalisationResult {
  double error_at_10 = -1.0;          // the position error when 10, 20 and 30 m had been travelled (negative: not reached)
  double error_at_20 = -1.0;
  double error_at_30 = -1.0;
  double max_error = 0.0;
  int fixes = 0;
  double distance = 0.0;              // true distance travelled
  double truth_left_at = -1.0;        // the distance travelled when the TRUE position first passed the edge of the corridor (negative: never)
  bool stopped = false;
  double stopped_at = -1.0;           // the distance travelled when the buffer made the robot brake
  bool truth_outside_at_stop = false; // the true position was already outside the corridor when it began to brake
  double final_lateral = 0.0;         // the true y at the end
};

// Drive straight ahead (equal wheel distances asked of both wheels) for `seconds` in steps of 0.1 s. The truth is moved by the real wheels (with the WheelError); the robot's belief by the nominal ones with rc::nav::Odometry. Every step in this order:
//  - move the truth and the belief; keep the largest position error and the error at 10, 20 and 30 m of true distance;
//  - if there is a beacon and its period has passed since the last fix (and it has not died): a fix of the true position plus noise of beacon_sigma on each axis (x first), through apply_fix with a rc::nav::Filter1D on each of x and y (motion variance 0.0004 a step); the metres since the last fix go to zero;
//  - if use_buffer: the believed position inside the corridor by at least the buffer (rc::safety::fence_state with that buffer): no? then the robot begins to brake (1 m/s^2) and does not stop braking;
//  - note the distance at which the truth first leaves the corridor.
inline LocalisationResult run_localisation(const LocalisationConfig& cfg, double seconds) {
  LocalisationResult r;
  const double dt = 0.1;
  const double base = 0.3;
  rc::sim::Pose start;
  rc::nav::Odometry belief(base, start);
  rc::nav::Odometry truth(base * cfg.error.base_scale, start);
  rc::nav::Filter1D fx(rc::nav::Estimate{0.0, 0.01}, 0.0004);
  rc::nav::Filter1D fy(rc::nav::Estimate{0.0, 0.01}, 0.0004);
  LocalisationNoise noise;
  double v = cfg.speed;
  double since_fix = 0.0;
  double last_fix = 0.0;
  bool braking = false;
  const double edge = cfg.half_width;
  const int steps = static_cast<int>(seconds / dt);
  double prev_x = 0.0;
  double prev_y = 0.0;
  for (int i = 0; i < steps; ++i) {
    const double t = i * dt;
    if (braking) v = rc::control::rate_limit(v, 0.0, 1.0 * dt);
    const double d = v * dt;
    const rc::sim::Pose before = belief.pose();
    const rc::sim::Pose tp = truth.update(d * cfg.error.left_scale, d * cfg.error.right_scale);
    const rc::sim::Pose bp = belief.update(d, d);
    fx.predict(bp.x - before.x);
    fy.predict(bp.y - before.y);
    r.distance += std::hypot(tp.x - prev_x, tp.y - prev_y);
    prev_x = tp.x;
    prev_y = tp.y;
    since_fix += d;
    r.max_error = std::max(r.max_error, position_error(bp, tp));
    if (r.error_at_10 < 0.0 && r.distance >= 10.0) r.error_at_10 = position_error(bp, tp);
    if (r.error_at_20 < 0.0 && r.distance >= 20.0) r.error_at_20 = position_error(bp, tp);
    if (r.error_at_30 < 0.0 && r.distance >= 30.0) r.error_at_30 = position_error(bp, tp);
    const bool beacon_alive = cfg.beacon_period > 0.0 && !(cfg.beacon_dead_at >= 0.0 && t >= cfg.beacon_dead_at);
    if (beacon_alive && t - last_fix >= cfg.beacon_period) {
      const double zx = tp.x + cfg.beacon_sigma * noise.gauss();
      const double zy = tp.y + cfg.beacon_sigma * noise.gauss();
      apply_fix(fx, fy, zx, zy, cfg.beacon_sigma, belief);
      ++r.fixes;
      last_fix = t;
      since_fix = 0.0;
    }
    if (cfg.use_buffer && !braking) {
      const std::vector<rc::safety::Point> corridor = {{-1.0, -edge}, {1000.0, -edge}, {1000.0, edge}, {-1.0, edge}};
      const double buffer = stop_buffer(cfg.stopping_distance, cfg.assumed_drift, since_fix);
      const rc::safety::FenceState s = rc::safety::fence_state(corridor, {belief.pose().x, belief.pose().y}, buffer, 0.0);
      if (s != rc::safety::FenceState::inside && s != rc::safety::FenceState::warning) {
        braking = true;
        r.stopped = true;
        r.stopped_at = r.distance;
        r.truth_outside_at_stop = std::fabs(tp.y) > edge;
      }
    }
    if (r.truth_left_at < 0.0 && std::fabs(tp.y) > edge) r.truth_left_at = r.distance;
    r.final_lateral = tp.y;
    if (braking && v <= 1e-9) break;
  }
  return r;
}

#endif  // LESSON_SOLUTION_HPP
