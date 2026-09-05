#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/control/feedforward.hpp>

using rc::control::Setpoint;

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
  // TODO 1: plan the phases.
  //
  // Speed up at the limit, hold the top speed, slow down at the limit. Except
  // when the move is too short to reach the top speed, and then there is no
  // middle phase and the profile is a triangle.
  //
  // The distance that decides it is exactly top_speed squared over the
  // acceleration, because that is what speeding up and slowing down again
  // costs. At or above it:
  //
  //     peak   = top_speed
  //     ramp   = top_speed / acceleration
  //     cruise = (distance - top_speed^2 / acceleration) / top_speed
  //
  // Below it there is no cruise at all, and the peak is whatever speeding up
  // over half the distance reaches, which is sqrt(acceleration * distance).
  //
  // Work in a positive distance and remember the direction separately, so a
  // move backwards is the same move with every sign turned round. And refuse a
  // top speed or an acceleration that is not positive, rather than dividing by
  // it.
  Trapezoid(double distance, double top_speed, double acceleration) {
    (void)distance;
    (void)top_speed;
    (void)acceleration;
  }

  double duration() const { return 2.0 * ramp_ + cruise_; }
  double ramp_time() const { return ramp_; }
  double cruise_time() const { return cruise_; }
  double peak_speed() const { return peak_ * direction_; }
  double acceleration() const { return acceleration_ * direction_; }
  bool cruises() const { return cruise_ > 0.0; }

  // TODO 2: where the target is at a given moment, and what it is doing.
  //
  // Three phases, each with a position, a velocity and an acceleration:
  //
  //   while speeding up      0.5 a t^2,          a t,          a
  //   while cruising         d1 + v s,           v,            0
  //   while slowing down     ... ,               v - a s,     -a
  //
  // where d1 is the distance covered by the first ramp and s is the time since
  // the current phase began.
  //
  // Outside the profile it is at one end or the other, standing still. That
  // matters as much as the middle: a caller that runs its clock past the end
  // must get the destination and a velocity of zero, not an extrapolation.
  //
  // Multiply all three by the direction at the end.
  Setpoint at(double t) const {
    (void)t;
    return Setpoint{};
  }

  // TODO 3: the same move, made to take longer.
  //
  // Stretching time by a factor divides every velocity by it and every
  // acceleration by its square, so the shape is preserved exactly. Build a new
  // profile over the same distance with the peak speed and the acceleration
  // scaled that way.
  //
  // That is what lesson 13-04 asked for when a singularity limits how fast one
  // direction can be driven: slow the whole path rather than one axis of it, or
  // the shape being drawn is distorted.
  //
  // Refuse a duration shorter than this profile's own, returning this profile
  // unchanged. The machine cannot do it, and handing back something it cannot
  // follow only moves the failure downstream.
  Trapezoid scaled_to(double seconds) const {
    (void)seconds;
    return *this;
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

#endif  // LESSON_SOLUTION_HPP
