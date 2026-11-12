// rc/vehicle/odometry.hpp
//
// Wheel odometry, from lesson 23-07.
//
// WheelOdometry takes the distance each side's wheels rolled in a step and works out the move from an effective track width (the real track times the ICR coefficient, for a skid steer; it takes in whatever else makes a turn come out as it does) and a distance scale (one less the slip, or a calibrated wheel radius) that applies to the distance only. AckermannOdometry takes the distance the rear wheels rolled and the steering angle measured, less its zero offset, and works out the move from the wheelbase. Both integrate on the heading at the middle of the step.
//
// With the parameters of the model a square of 10 m sides closed within 0.023 m; a track width 3 percent too small made a full turn read 11.1 degrees wrong; a wheel scale 2 percent out made 10 m read 0.20 m long; against losses of 2 percent slip and an efficiency of 0.9 the ideal parameters believed the square ended 0.02 m from home when it ended 4.6 m away, and the calibrated ones were 0.000 m out; and a steering sensor 1 degree off zero made a car driven straight for 20 m believe it had turned 61 degrees and had moved 9.6 m sideways.

#ifndef RC_RC_VEHICLE_ODOMETRY_HPP
#define RC_RC_VEHICLE_ODOMETRY_HPP

#include <cmath>
#include <rc/vehicle/diff_slip.hpp>

namespace rc {
namespace vehicle {

// Odometry for a vehicle that steers by driving its sides at different speeds.
class WheelOdometry {
 public:
  // effective_track: the real track times the ICR coefficient (or divided by the turn efficiency), which takes in whatever else makes a turn come out other than the wheels say; scale: the ground travel per metre of wheel travel, for the distance only.
  WheelOdometry(double effective_track, double scale) : b_(effective_track), scale_(scale) {}
  void reset(const Pose2& p) { pose_ = p; }
  const Pose2& pose() const { return pose_; }

  // The distance each side's wheels rolled in the step (metres, negative for backwards).
  void update(double left_m, double right_m) {
    const double ds = scale_ * 0.5 * (left_m + right_m);
    const double dtheta = (right_m - left_m) / b_;
    const double mid = pose_.theta + 0.5 * dtheta;
    pose_.x += ds * std::cos(mid);
    pose_.y += ds * std::sin(mid);
    pose_.theta += dtheta;
  }

 private:
  double b_;
  double scale_;
  Pose2 pose_;
};

// Odometry for a car: the rear wheels give the distance and the front ones are steered, and the steering angle is measured by a sensor that may not read zero when the wheels point straight.
class AckermannOdometry {
 public:
  AckermannOdometry(double wheelbase, double steer_zero_offset) : l_(wheelbase), offset_(steer_zero_offset) {}
  void reset(const Pose2& p) { pose_ = p; }
  const Pose2& pose() const { return pose_; }

  void update(double rear_m, double steer_measured) {
    const double delta = steer_measured - offset_;
    const double dtheta = rear_m * std::tan(delta) / l_;
    const double mid = pose_.theta + 0.5 * dtheta;
    pose_.x += rear_m * std::cos(mid);
    pose_.y += rear_m * std::sin(mid);
    pose_.theta += dtheta;
  }

 private:
  double l_;
  double offset_;
  Pose2 pose_;
};


}  // namespace vehicle
}  // namespace rc

#endif
