#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>

#include <rc/core/compat.hpp>

// A mass that is pushed, with a little friction and possibly a steady load.
//
// This is what a joint or a wheel actually is, and the difference from a lag
// matters here: a first order lag cannot overshoot however hard it is driven,
// so a controller tuned against one teaches nothing about the thing that makes
// tuning difficult. A mass can, because it has momentum to carry past the
// target.
//
// The load is what a steady offset is made of. Gravity on a vertical joint, a
// spring, a belt under tension: something that needs a constant force just to
// stay still. Without one, proportional control alone arrives exactly, and the
// integral term has nothing to do but harm.
class Mass {
 public:
  Mass(double mass, double damping, double load)
      : mass_(mass <= 0.0 ? 1.0 : mass), damping_(damping), load_(load) {}

  double step(double force, double dt) {
    const double acceleration = (force - load_ - damping_ * velocity_) / mass_;
    velocity_ += acceleration * dt;
    position_ += velocity_ * dt;
    return position_;
  }

  double position() const { return position_; }
  double velocity() const { return velocity_; }

 private:
  double mass_ = 1.0;
  double damping_ = 0.0;
  double load_ = 0.0;
  double position_ = 0.0;
  double velocity_ = 0.0;
};

// What a step response is worth knowing about. Four numbers, and a loop is
// tuned when all four are acceptable rather than when any one is best.
struct StepResponse {
  double rise_time = -1.0;       // ten percent to ninety, negative if never reached
  double overshoot = 0.0;        // how far past the target, as a fraction of it
  double settling_time = -1.0;   // when it last left the band, negative if it never stopped
  double final_error = 0.0;      // what is still missing at the end
};

// band is a fraction of the target: 0.02 means within two percent.
inline StepResponse analyse(rc::span<const double> output, double target, double dt,
                            double band) {
  StepResponse response;
  if (output.size() == 0) return response;

  // Overshoot is measured from the target, not from zero. A peak of 1.8 against
  // a target of 1.0 is eighty percent past it, not a hundred and eighty.
  // TODO: how far past the target it went, as a fraction of the target.
  //
  // Measured from the target, not from zero: a peak of 1.8 against a target of
  // 1.0 is eighty percent past it, not a hundred and eighty. A response that
  // never reached the target overshot by nothing, not by a negative amount.

  // TODO: how long it took to get from a tenth of the target to nine tenths.
  //
  // A response that never reached nine tenths has no rise time, and saying so
  // is different from saying zero.

  // Settling is the last time it was outside the band, not the first time it
  // was inside. A response oscillating through the target passes through the
  // band on the way past, and reporting that as settled says a loop that is
  // still ringing has already finished.
  // TODO: when it last left the band, and stayed inside afterwards.
  //
  // The last time out, not the first time in. A response oscillating through
  // the target passes through the band on the way past, and reporting that as
  // settled says a loop that is still ringing has already finished, which is
  // the wrong answer by several seconds and in the flattering direction.
  //
  // A response still outside the band on its final sample never settled at
  // all, and that is a different answer from settling at the end of the
  // recording.
  //
  // TODO: and what is still missing at the end.
  (void)band;
  (void)dt;
  return response;
}

#endif  // LESSON_SOLUTION_HPP
