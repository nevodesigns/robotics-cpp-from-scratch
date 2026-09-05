// rc/control/profile.hpp
//
// A setpoint that moves at a speed the machine actually has, from lesson 14-06.
//
// A step tells the loop to be somewhere else immediately, which asks for an
// infinite velocity and gets whatever the actuator can produce until the error
// comes down. Measured on the loop from 14-05, commanding a one metre move:
//
//   a step                       peak command 20.2 N   worst error 0.9223 m
//   a profile with feedforward   peak command  1.4 N   worst error 0.1987 m
//
// and the profile's remaining error is exactly the standing lag 14-05 measured,
// kd * v / kp, which that lesson says how to remove.
//
// A profile also produces the two numbers feedforward needs. It knows its
// velocity and acceleration at every instant because it computed them; a step
// knows neither.
//
// The case to get right is the short move. Below top_speed^2 / acceleration
// there is no cruise phase at all, and a planner that assumes three phases
// computes a negative cruise time, arrives at exactly the right place, and asks
// for an instantaneous 0.3 m/s change of velocity on the way.

#ifndef RC_CONTROL_PROFILE
#define RC_CONTROL_PROFILE

#include <cmath>

#include <rc/control/feedforward.hpp>

namespace rc {
namespace control {

// A setpoint that moves at a speed the machine actually has.
//
// A step tells the loop to be somewhere else immediately, which asks for
// infinite velocity and gets whatever the actuator can produce until the error
// comes down. Measured on the loop from 14-05, commanding a one metre move: a
// step peaked at 20.2 newtons and was 0.92 m behind at its worst, and the same
// move through a profile peaked at 1.4 and was 0.20 m behind, which is the
// standing lag 14-05 already explained.
//
// It also produces the two numbers feedforward needs. A profile knows its
// velocity and acceleration at every instant because it computed them; a step
// knows neither.
class Trapezoid {
 public:
  // Speed up at the limit, hold the top speed, slow down at the limit.
  //
  // Except when the move is too short to reach the top speed, and then there is
  // no middle phase and the profile is a triangle. The distance that decides it
  // is exactly top_speed squared over the acceleration, because that is what
  // speeding up and slowing down again costs.
  Trapezoid(double distance, double top_speed, double acceleration) {
    if (!(top_speed > 0.0) || !(acceleration > 0.0)) return;

    // Work in a positive distance and remember which way it was.
    direction_ = distance < 0.0 ? -1.0 : 1.0;
    distance_ = std::fabs(distance);
    if (distance_ == 0.0) return;

    acceleration_ = acceleration;
    const double needed = top_speed * top_speed / acceleration;

    if (distance_ >= needed) {
      peak_ = top_speed;
      ramp_ = top_speed / acceleration;
      cruise_ = (distance_ - needed) / top_speed;
    } else {
      // The top speed is never reached, so the peak is whatever speeding up
      // over half the distance produces.
      peak_ = std::sqrt(acceleration * distance_);
      ramp_ = peak_ / acceleration;
      cruise_ = 0.0;
    }
  }

  double duration() const { return 2.0 * ramp_ + cruise_; }
  double ramp_time() const { return ramp_; }
  double cruise_time() const { return cruise_; }
  double peak_speed() const { return peak_ * direction_; }
  double acceleration() const { return acceleration_ * direction_; }
  bool cruises() const { return cruise_ > 0.0; }

  // Where the target is at a given moment, and what it is doing.
  //
  // Outside the profile it is at one end or the other, standing still. That
  // matters as much as the middle: a caller that runs the clock past the end
  // must get the destination and a velocity of zero, not an extrapolation.
  Setpoint at(double t) const {
    Setpoint point;
    if (distance_ == 0.0 || acceleration_ == 0.0) return point;

    if (t <= 0.0) return point;
    if (t >= duration()) {
      point.position = distance_ * direction_;
      return point;
    }

    if (t < ramp_) {
      point.position = 0.5 * acceleration_ * t * t;
      point.velocity = acceleration_ * t;
      point.acceleration = acceleration_;
    } else if (t < ramp_ + cruise_) {
      const double s = t - ramp_;
      point.position = ramped_distance() + peak_ * s;
      point.velocity = peak_;
      point.acceleration = 0.0;
    } else {
      const double s = t - ramp_ - cruise_;
      point.position =
          ramped_distance() + peak_ * cruise_ + peak_ * s - 0.5 * acceleration_ * s * s;
      point.velocity = peak_ - acceleration_ * s;
      point.acceleration = -acceleration_;
    }

    point.position *= direction_;
    point.velocity *= direction_;
    point.acceleration *= direction_;
    return point;
  }

  // The same move, made to take longer.
  //
  // Stretching time by a factor divides every velocity by it and every
  // acceleration by its square, so the shape is preserved exactly. That is what
  // lesson 13-04 asked for when a singularity limits how fast one direction can
  // be driven: slow the whole path rather than one axis of it, or the shape
  // being drawn is distorted.
  //
  // A duration shorter than this profile's own is refused, because the machine
  // cannot do it, and returning something it cannot follow would only move the
  // failure downstream.
  Trapezoid scaled_to(double seconds) const {
    if (duration() <= 0.0 || seconds <= duration()) return *this;

    const double stretch = seconds / duration();
    return Trapezoid(distance_ * direction_, peak_ / stretch,
                     acceleration_ / (stretch * stretch));
  }

 private:
  double ramped_distance() const { return 0.5 * acceleration_ * ramp_ * ramp_; }

  double distance_ = 0.0;
  double direction_ = 1.0;
  double acceleration_ = 0.0;
  double peak_ = 0.0;
  double ramp_ = 0.0;
  double cruise_ = 0.0;
};

}  // namespace control
}  // namespace rc

#endif  // RC_CONTROL_PROFILE
