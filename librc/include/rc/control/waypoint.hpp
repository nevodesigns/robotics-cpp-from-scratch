// rc/control/waypoint.hpp
//
// The waypoint driver from lesson 14-03, graduated, and the first thing in this
// curriculum built out of four phases at once: the model from phase 00, the
// angle handling from phase 01, the controller from 14-01 and the watchdog from
// 14-02.
//
// Two decisions in it are the difference between arriving and circling. It does
// not drive forward while badly aimed, because driving in the wrong direction
// at speed makes the heading error worse faster than the controller can correct
// it. And it slows on approach, because a controller that arrives at full speed
// overshoots and comes back, which is E-CTRL-0005.

#ifndef RC_CONTROL_WAYPOINT_HPP
#define RC_CONTROL_WAYPOINT_HPP

#include <algorithm>
#include <cmath>

#include <rc/control/pid.hpp>
#include <rc/control/safety.hpp>
#include <rc/sim/diff_drive.hpp>

namespace rc {
namespace control {

struct WheelSpeeds {
  double left = 0.0;
  double right = 0.0;
};

struct DriverConfig {
  double wheel_base = 0.30;
  double max_wheel_speed = 0.6;
  double max_forward_speed = 0.45;
  double arrival_tolerance = 0.05;
  double turn_first_threshold = 0.5;
  double target_timeout = 0.5;
};

// The signed shortest rotation from where the robot faces to where the target
// lies. Wrapped, so a target just behind and to the left is a small left turn
// rather than most of a circle.
inline double heading_error(const rc::sim::Pose& pose, const rc::sim::Pose& target) {
  const double desired = std::atan2(target.y - pose.y, target.x - pose.x);
  return rc::sim::wrap_angle(desired - pose.theta);
}

// Inverse of the model in rc::sim. Forward speed is the average of the wheels
// and turn rate is their difference over the base, so going the other way is
// half the difference added to and subtracted from the average.
inline WheelSpeeds to_wheel_speeds(double forward, double turn, double wheel_base) {
  const double half_difference = turn * wheel_base / 2.0;
  WheelSpeeds speeds;
  speeds.left = forward - half_difference;
  speeds.right = forward + half_difference;
  return speeds;
}

inline double distance_to(const rc::sim::Pose& pose, const rc::sim::Pose& target) {
  return std::hypot(target.x - pose.x, target.y - pose.y);
}

class WaypointDriver {
 public:
  explicit WaypointDriver(DriverConfig config = DriverConfig{})
      : config_(config),
        heading_(2.0, 0.0, 0.1, -2.0, 2.0),
        watchdog_(config.target_timeout, 0.0) {}

  // A target arriving from outside, a planner or an operator. Feeding the
  // watchdog here is what makes a target perishable: if they stop arriving, the
  // robot stops rather than driving on towards a stale goal.
  void set_target(const rc::sim::Pose& target, double now) {
    target_ = target;
    watchdog_.feed(now);
  }

  WheelSpeeds command(const rc::sim::Pose& pose, double now, double dt) {
    // The safe state first, before any control arithmetic. A stale target, or
    // one that never arrived at all, means stop.
    if (watchdog_.expired(now)) return WheelSpeeds{};

    if (arrived(pose)) return WheelSpeeds{};

    const double error = heading_error(pose, target_);

    // The setpoint is zero error, and the measurement is the error itself
    // negated, so that a positive error asks for a positive turn rate.
    const double turn = heading_.update(0.0, -error, dt);

    // Do not drive forward while badly aimed, or the robot arcs away from the
    // target before it comes round. Between aimed and badly aimed the forward
    // speed fades rather than switching, because a step change in command is
    // exactly what the rate limiter in lesson 14-02 exists to prevent.
    const double aim = 1.0 - std::min(1.0, std::fabs(error) / config_.turn_first_threshold);

    // Slow down on approach so the robot settles rather than overshooting and
    // circling back, which is the classic look of an untuned waypoint follower.
    const double approach = std::min(1.0, distance_to(pose, target_) / 0.4);

    const double forward = config_.max_forward_speed * aim * approach;

    WheelSpeeds speeds = to_wheel_speeds(forward, turn, config_.wheel_base);
    speeds.left = clamp_wheel(speeds.left);
    speeds.right = clamp_wheel(speeds.right);
    return speeds;
  }

  bool arrived(const rc::sim::Pose& pose) const {
    return distance_to(pose, target_) <= config_.arrival_tolerance;
  }

  const DriverConfig& config() const { return config_; }

 private:
  double clamp_wheel(double speed) const {
    return std::max(-config_.max_wheel_speed, std::min(config_.max_wheel_speed, speed));
  }

  DriverConfig config_;
  rc::control::Pid heading_;
  rc::control::Watchdog watchdog_;
  rc::sim::Pose target_;
};

}  // namespace control
}  // namespace rc

#endif  // RC_CONTROL_WAYPOINT_HPP
