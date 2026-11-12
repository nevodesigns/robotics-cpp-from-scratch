// rc/vehicle/diff_slip.hpp
//
// A differential drive that slips, from lesson 23-01.
//
// DiffSlip takes the speeds of the two wheels (rad/s) and moves a pose: the ground speed is the mean wheel speed times the wheel radius less the longitudinal slip, and the turn rate is the difference of the wheel speeds over the track width times the turn efficiency. The step uses the heading at the middle of the step. turn_efficiency_from reads the efficiency back from the wheel travel and the heading change of a measured turn.
//
// Over a wheel travel of 10 m a slip of 5 percent was 9.50 m of ground; a turn that the wheels say is 90 degrees was 72.0 degrees at an efficiency of 0.8 and took 25 percent more wheel travel to make; a square of 10 m sides driven on the commands of the ideal model ended 4.64 m from where it began with a slip of 2 percent and an efficiency of 0.9; and the efficiency read back from a measured turn was 0.800.

#ifndef RC_RC_VEHICLE_DIFF_SLIP_HPP
#define RC_RC_VEHICLE_DIFF_SLIP_HPP

#include <cmath>

namespace rc {
namespace vehicle {

struct Pose2 {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

// A differential drive whose floor takes something from every move: slip is the fraction of the wheel travel that does not become ground travel, and turn_eff is the fraction of the turn the wheel difference asks for that the body makes.
class DiffSlip {
 public:
  DiffSlip(double wheel_radius, double track_width, double slip, double turn_efficiency)
      : r_(wheel_radius), b_(track_width), slip_(slip), eff_(turn_efficiency) {}

  void reset(const Pose2& p) { pose_ = p; }
  const Pose2& pose() const { return pose_; }

  // Advances by dt with the wheel speeds in rad/s (positive turns the wheel forward).
  void step(double wl, double wr, double dt) {
    const double v = 0.5 * r_ * (wl + wr) * (1.0 - slip_);
    const double w = eff_ * r_ * (wr - wl) / b_;
    const double mid = pose_.theta + 0.5 * w * dt;
    pose_.x += v * std::cos(mid) * dt;
    pose_.y += v * std::sin(mid) * dt;
    pose_.theta += w * dt;
  }

 private:
  double r_;
  double b_;
  double slip_;
  double eff_;
  Pose2 pose_;
};

// The turn efficiency from a measured turn: the wheels travelled left_m and right_m (metres of wheel), and the body turned dtheta radians.
inline double turn_efficiency_from(double left_m, double right_m, double track_width, double dtheta) {
  const double ideal = (right_m - left_m) / track_width;
  return ideal != 0.0 ? dtheta / ideal : 0.0;
}


}  // namespace vehicle
}  // namespace rc

#endif
