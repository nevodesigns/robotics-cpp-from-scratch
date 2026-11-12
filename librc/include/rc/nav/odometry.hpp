// rc/nav/odometry.hpp
//
// The pose estimate from lesson 16-01, graduated.
//
// Dead reckoning: no landmark, no map, no satellite. Every step adds to the
// estimate and nothing ever corrects it, so every error that enters stays in,
// and the estimate is only as good as the last time somebody told it the truth.
// Everything else in this phase is a way of telling it.
//
// Which error costs what depends on the path, which is the finding worth
// carrying. Measured over a hundred metres on the same robot: a wheel base one
// percent wrong costs nothing at all driving straight and 1.06 metres with turns
// in it, because the base only ever divides the difference between the
// wheels. A heading one degree wrong costs 1.75 metres straight and almost
// nothing around a closed loop. A calibration run that only drives one kind of
// path cannot see the errors the other kind exposes.
//
// And the estimate cuts every corner, because each step is a straight line
// where the robot drove an arc. Measured on a quarter circle, the error is
// close to the length of one step, and halves when the step does, which is the
// argument for integrating often rather than for a cleverer formula.

#ifndef RC_NAV_ODOMETRY_HPP
#define RC_NAV_ODOMETRY_HPP

#include <cmath>

#include <rc/math/angles.hpp>
#include <rc/sim/diff_drive.hpp>

namespace rc {
namespace nav {

// Where the robot thinks it is, from wheel measurements alone.
//
// This is dead reckoning: no landmark, no map, no satellite. Every step adds
// to the estimate and nothing ever corrects it, so every error that enters
// stays in and the estimate is only as good as the last time somebody told it
// the truth.
//
// Which is not a reason to distrust it. It is the only thing that keeps working
// when the lights go out, it is available at the rate the encoders report, and
// every method that does correct it corrects this.
// How a step of the wheels is turned into a move of the pose. start_heading is the original: the whole step is taken along the heading at the START of the step, which is exact for a straight
// step and wrong by an amount that goes with the size of the turn in each step. midpoint takes it along the heading halfway through the step, and arc along the exact circle the two wheels
// describe. Hardened after phase 21 (docs/BUILD-LOG.md, "Hardening pass"): over a half circle of radius 1 m start_heading was 0.20, 0.10 and 0.05 m out at 0.2, 0.1 and 0.05 rad a step.
// It stays the default so that every number measured with it is the same.
enum class Integration { start_heading, midpoint, arc };

class Odometry {
 public:
  // The wheel base is the one number here that is not measured, it is believed.
  // It comes off a drawing or a tape measure, and lesson 16-01 measures what
  // believing it wrongly costs.
  Odometry(double wheel_base, const rc::sim::Pose& start, Integration integration = Integration::start_heading)
      : wheel_base_(wheel_base <= 0.0 ? 1.0 : wheel_base), pose_(start), integration_(integration) {}

  Integration integration() const { return integration_; }

  // The distances each wheel rim travelled since the last call, in metres.
  //
  // Rim travel, not ground travel. An encoder counts wheel rotation and cannot
  // tell the difference, which is exactly why slip is invisible to it and shows
  // up later as a position that is quietly wrong.
  rc::sim::Pose update(double left, double right) {
    const double forward = (left + right) / 2.0;
    const double turn = (right - left) / wheel_base_;

    // Along the heading held at the start of the step. This is a straight line
    // where the robot drove an arc, and the two differ by less the shorter the
    // step is, which is the argument for integrating often rather than for a
    // cleverer formula.
    if (integration_ == Integration::arc && std::fabs(turn) > 1e-9) {
      // The exact circle: radius forward / turn, and the position on it after turning through `turn`.
      const double radius = forward / turn;
      pose_.x += radius * (std::sin(pose_.theta + turn) - std::sin(pose_.theta));
      pose_.y += radius * (std::cos(pose_.theta) - std::cos(pose_.theta + turn));
    } else {
      const double heading = integration_ == Integration::midpoint ? pose_.theta + turn / 2.0 : pose_.theta;
      pose_.x += forward * std::cos(heading);
      pose_.y += forward * std::sin(heading);
    }

    // Wrapped, or the heading grows without limit and every comparison against
    // it becomes wrong in a way that depends on how long the robot has been on.
    pose_.theta = rc::math::wrap_angle(pose_.theta + turn);

    travelled_ += std::fabs(forward);
    return pose_;
  }

  const rc::sim::Pose& pose() const { return pose_; }
  double travelled() const { return travelled_; }
  double wheel_base() const { return wheel_base_; }

  // Correcting the estimate from outside is the whole of the rest of this
  // phase. The distance travelled keeps running, because how far the robot has
  // driven since the last correction is what says how much to trust it.
  void correct(const rc::sim::Pose& truth) { pose_ = truth; }

 private:
  double wheel_base_ = 1.0;
  rc::sim::Pose pose_;
  Integration integration_ = Integration::start_heading;
  double travelled_ = 0.0;
};

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_ODOMETRY_HPP
