#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/math/angles.hpp>
#include <rc/sim/diff_drive.hpp>

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
class Odometry {
 public:
  // The wheel base is the one number here that is not measured, it is believed.
  // It comes off a drawing or a tape measure, and lesson 16-01 measures what
  // believing it wrongly costs.
  Odometry(double wheel_base, const rc::sim::Pose& start)
      : wheel_base_(wheel_base <= 0.0 ? 1.0 : wheel_base), pose_(start) {}

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
    pose_.x += forward * std::cos(pose_.theta);
    pose_.y += forward * std::sin(pose_.theta);

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
  double travelled_ = 0.0;
};

#endif  // LESSON_SOLUTION_HPP
