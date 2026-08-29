#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>

#include <rc/control/pid.hpp>
#include <rc/control/safety.hpp>
#include <rc/sim/diff_drive.hpp>

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
// lies. Positive means anticlockwise, and it must be wrapped, so a target just
// behind and to the left is a small left turn rather than most of a circle.
inline double heading_error(const rc::sim::Pose& pose, const rc::sim::Pose& target) {
  // TODO: std::atan2 of the difference in y over the difference in x gives the
  // direction to the target. Subtract the robot's heading, then wrap.
  (void)pose;
  (void)target;
  return 0.0;
}

// The inverse of the model you wrote in lesson 00-04. Given a forward speed and
// a turn rate, what must each wheel do?
inline WheelSpeeds to_wheel_speeds(double forward, double turn, double wheel_base) {
  // TODO: forward speed is the average of the wheels and turn rate is their
  // difference over the base, so going the other way is half the difference
  // added to and subtracted from the average.
  (void)forward;
  (void)turn;
  (void)wheel_base;
  return WheelSpeeds{};
}

inline double distance_to(const rc::sim::Pose& pose, const rc::sim::Pose& target) {
  return std::hypot(target.x - pose.x, target.y - pose.y);
}

class WaypointDriver {
 public:
  explicit WaypointDriver(DriverConfig config = DriverConfig{})
      : config_(config),
        // Proportional and a little derivative on heading. No integral: a
        // steady heading offset is not a thing a driving robot suffers from,
        // and an integral term here mostly stores up trouble for the next turn.
        heading_(2.0, 0.0, 0.1, -2.0, 2.0),
        watchdog_(config.target_timeout, 0.0) {}

  // A target arriving from outside, a planner or an operator.
  void set_target(const rc::sim::Pose& target, double now) {
    target_ = target;
    // TODO: feed the watchdog here. That is what makes a target perishable.
  }

  WheelSpeeds command(const rc::sim::Pose& pose, double now, double dt) {
    // TODO, in this order:
    //
    // 1. if the watchdog has expired, stop. Before any control arithmetic, and
    //    remember that a watchdog never fed counts as expired.
    // 2. if already arrived, stop.
    // 3. work out the heading error, and ask the PID for a turn rate. The
    //    setpoint is zero error, so pass 0.0 as the setpoint and the negated
    //    error as the measurement.
    // 4. choose a forward speed. Do not drive forward while badly aimed, or the
    //    robot arcs away before it comes round. Fade rather than switch, and
    //    slow down on approach so it settles instead of circling.
    // 5. convert to wheel speeds and clamp each wheel to max_wheel_speed.
    (void)pose;
    (void)now;
    (void)dt;
    return WheelSpeeds{};
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

#endif  // LESSON_SOLUTION_HPP
