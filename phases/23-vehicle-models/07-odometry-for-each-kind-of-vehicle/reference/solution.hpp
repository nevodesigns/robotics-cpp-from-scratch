#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/vehicle/diff_slip.hpp>

using namespace rc::vehicle;

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


#endif
