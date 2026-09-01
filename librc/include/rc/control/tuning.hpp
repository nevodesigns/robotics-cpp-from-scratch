// rc/control/tuning.hpp
//
// The step response measurements from lesson 14-04, graduated, with the plant
// to try gains against.
//
// A loop is tuned when four numbers are acceptable, not when one is best, and
// two of the four have a trap in their definition. Overshoot is measured from
// the target rather than from zero, and a response that stopped short overshot
// by nothing rather than by a negative amount. Settling is the last exit from
// the band rather than the first entry, because a ringing response passes
// through the band on its way past the target, and the wrong version reports
// success earlier the worse the ringing is.
//
// The plant is a mass rather than a lag, because a first order lag cannot
// overshoot however hard it is driven, and a controller tuned against one has
// not met the thing that makes tuning difficult.

#ifndef RC_CONTROL_TUNING_HPP
#define RC_CONTROL_TUNING_HPP

#include <cmath>
#include <cstddef>

#include <rc/core/compat.hpp>

namespace rc {
namespace control {

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
  double peak = output[0];
  for (std::size_t i = 1; i < output.size(); ++i)
    if (output[i] > peak) peak = output[i];

  response.overshoot = target > 0.0 ? (peak - target) / target : 0.0;
  if (response.overshoot < 0.0) response.overshoot = 0.0;

  bool reached_ten = false;
  double time_at_ten = 0.0;
  for (std::size_t i = 0; i < output.size(); ++i) {
    const double time = static_cast<double>(i) * dt;
    if (!reached_ten && output[i] >= 0.1 * target) {
      reached_ten = true;
      time_at_ten = time;
    }
    if (reached_ten && response.rise_time < 0.0 && output[i] >= 0.9 * target) {
      response.rise_time = time - time_at_ten;
    }
  }

  // Settling is the last time it was outside the band, not the first time it
  // was inside. A response oscillating through the target passes through the
  // band on the way past, and reporting that as settled says a loop that is
  // still ringing has already finished.
  long last_outside = -1;
  const double tolerance = std::fabs(band * target);
  for (std::size_t i = 0; i < output.size(); ++i)
    if (std::fabs(output[i] - target) > tolerance) last_outside = static_cast<long>(i);

  // Still outside on the final sample means it never settled at all, which is
  // a different answer from settling at the end of the recording.
  const bool settled = last_outside >= 0 &&
                       last_outside + 1 < static_cast<long>(output.size());
  if (last_outside < 0) response.settling_time = 0.0;
  else if (settled) response.settling_time = static_cast<double>(last_outside + 1) * dt;

  response.final_error = target - output[output.size() - 1];
  return response;
}

}  // namespace control
}  // namespace rc

#endif  // RC_CONTROL_TUNING_HPP
