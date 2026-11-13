// rc/drive/teleop_governor.hpp
//
// A person at the controls, from lesson 27-02.
//
// stop_speed is the speed from which a vehicle that goes on at that speed for a latency and then brakes at a deceleration stops within a distance: a (sqrt(T squared + 2 d / a) minus T). operator_speed_limit adds up what a person adds to the delay (the video's age, their reaction and the command's trip) and gives the speed they can be allowed to drive at for what they can see. TeleopGovernor takes the operator's requested speed and the distance to the nearest thing the vehicle itself senses (or none) and gives the speed to command: the request, dropped to zero if no command has arrived within the timeout, capped at the top speed and at the speed that stops short of the thing with a margin, and then ramped from the speed the vehicle has by an acceleration limit going up and a braking limit coming down.
//
// With a sight line of 6 m and a deceleration of 2 m/s2 the speed a person could be allowed was 4.00 m/s with 0.5 s of delay in all, 3.29 m/s with 1.0 s and 2.32 m/s with 2.0 s. After the commands stopped, a vehicle at 2 m/s went on 1.99 m with a timeout of 0.5 s and 4.99 m with 2.0 s. A request for 2 m/s took 1.33 s to be reached. With an obstacle 20 m ahead and the operator holding full speed the governed vehicle stopped 0.50 m short of it, and one with no governor on the obstacle went 58.7 m past where it was.

#ifndef RC_RC_DRIVE_TELEOP_GOVERNOR_HPP
#define RC_RC_DRIVE_TELEOP_GOVERNOR_HPP

#include <algorithm>
#include <cmath>
#include <string>

namespace rc {
namespace drive {

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


}  // namespace drive
}  // namespace rc

#endif
