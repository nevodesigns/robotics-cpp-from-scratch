#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <string>

// The speed from which a vehicle that goes on for `latency` seconds and then brakes at `decel` stops within `distance`.
inline double stop_speed(double distance, double latency, double decel) {
  if (distance <= 0.0 || decel <= 0.0) return 0.0;
  return decel * (std::sqrt(latency * latency + 2.0 * distance / decel) - latency);
}

// The speed an operator can be allowed for a sight line, given the age of the picture, their reaction time and the trip of the command.
inline double operator_speed_limit(double sight, double video_delay, double reaction, double command_delay, double decel) {
  return stop_speed(sight, video_delay + reaction + command_delay, decel);
}

struct TeleopLimits {
  double v_max = 2.0;     // m/s
  double accel = 1.5;     // m/s2 going up
  double decel = 2.0;     // m/s2 coming down
  double timeout = 0.5;   // s without a command before the vehicle stops
  double latency = 0.2;   // s the vehicle itself takes to start braking
  double margin = 0.5;    // m to stop short of an obstacle
};

class TeleopGovernor {
 public:
  explicit TeleopGovernor(const TeleopLimits& lim) : lim_(lim) {}

  // The operator's request, with the time it arrived.
  void request(double t, double v) {
    last_t_ = t;
    request_ = v;
    heard_ = true;
  }

  // The speed to command now. `obstacle` is the distance to what the vehicle senses ahead, or a negative number for nothing.
  double step(double t, double dt, double obstacle) {
    double target = request_;
    if (!heard_ || t - last_t_ > lim_.timeout) target = 0.0;
    target = std::max(0.0, std::min(target, lim_.v_max));
    if (obstacle >= 0.0) target = std::min(target, stop_speed(obstacle - lim_.margin, lim_.latency, lim_.decel));
    if (target > v_) {
      v_ = std::min(target, v_ + lim_.accel * dt);
    } else {
      v_ = std::max(target, v_ - lim_.decel * dt);
    }
    return v_;
  }

  double speed() const { return v_; }

 private:
  TeleopLimits lim_;
  double v_ = 0.0;
  double request_ = 0.0;
  double last_t_ = 0.0;
  bool heard_ = false;
};


#endif
