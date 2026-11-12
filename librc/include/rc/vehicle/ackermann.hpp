// rc/vehicle/ackermann.hpp
//
// A car-like vehicle, from lesson 23-03.
//
// CarGeometry is the wheelbase, the track and the largest steering angle; BicycleCar moves the rear axle's centre with x' = v cos theta, y' = v sin theta, theta' = v tan(delta) / L. turning_radius is L / tan(delta) and min_turning_radius is the same at the largest angle. ackermann_angles gives the angle of the inner and the outer front wheel for a circle of a given radius (atan(L / (R -/+ B/2))), so that all four wheels roll about one point. swept_radius_outer is how far the outer front wheel is from that point. curvature_to_steer turns a path curvature into a steering angle within the limit, and max_speed_in_turn is sqrt(mu g R).
//
// For a wheelbase of 0.3302 m (the F1TENTH car, from the course wiki of one of its teams), a track of 0.20 m and a largest angle of 24 degrees (the last two typical), the smallest turning circle was 0.742 m in radius, the front wheels were at 27.2 and 21.4 degrees (5.8 degrees apart), the outer wheel swept 0.904 m, the car needed a lane 0.062 m wider than itself, and on gravel the fastest it could go round that circle was 1.81 m/s.

#ifndef RC_RC_VEHICLE_ACKERMANN_HPP
#define RC_RC_VEHICLE_ACKERMANN_HPP

#include <algorithm>
#include <cmath>

namespace rc {
namespace vehicle {

// A car-like vehicle: steered front wheels and fixed rear ones.
struct CarGeometry {
  double wheelbase = 0.3302;   // L, metres (from the F1TENTH car, see the lesson)
  double track = 0.20;         // B, metres (typical)
  double max_steer = 0.4189;   // largest steering angle, radians: 24 degrees (typical)
};

// The kinematic bicycle model, with the pose at the middle of the rear axle: the two front wheels are taken to be one wheel at the middle of the front axle.
class BicycleCar {
 public:
  explicit BicycleCar(const CarGeometry& g) : g_(g) {}

  void reset(double x, double y, double theta) {
    x_ = x;
    y_ = y;
    theta_ = theta;
  }

  // Steps by dt at a speed v (m/s, negative to reverse) and a steering angle delta (radians), which is held within the limit.
  void step(double v, double delta, double dt) {
    const double d = std::max(-g_.max_steer, std::min(g_.max_steer, delta));
    const double rate = v * std::tan(d) / g_.wheelbase;
    const double mid = theta_ + 0.5 * rate * dt;
    x_ += v * std::cos(mid) * dt;
    y_ += v * std::sin(mid) * dt;
    theta_ += rate * dt;
  }

  double x() const { return x_; }
  double y() const { return y_; }
  double theta() const { return theta_; }

 private:
  CarGeometry g_;
  double x_ = 0.0;
  double y_ = 0.0;
  double theta_ = 0.0;
};

// The radius of the circle the rear axle's centre drives for a steering angle.
inline double turning_radius(double wheelbase, double delta) { return wheelbase / std::tan(delta); }

inline double min_turning_radius(const CarGeometry& g) { return turning_radius(g.wheelbase, g.max_steer); }

// The angles of the inner and the outer front wheel (towards and away from the centre of the circle) so that every wheel rolls about the same point, for a circle of radius R of the rear axle's centre.
inline void ackermann_angles(const CarGeometry& g, double radius, double& inner, double& outer) {
  inner = std::atan(g.wheelbase / (radius - 0.5 * g.track));
  outer = std::atan(g.wheelbase / (radius + 0.5 * g.track));
}

// How far the outer front wheel is from the centre of the circle, which is the outer edge of the lane the car needs.
inline double swept_radius_outer(const CarGeometry& g, double radius) {
  const double a = radius + 0.5 * g.track;
  return std::sqrt(a * a + g.wheelbase * g.wheelbase);
}

// The steering angle for a path of curvature kappa (1 / radius), held within the limit.
inline double curvature_to_steer(const CarGeometry& g, double kappa) {
  const double d = std::atan(g.wheelbase * kappa);
  return std::max(-g.max_steer, std::min(g.max_steer, d));
}

// The fastest the car can drive a circle of radius R on a surface of friction mu before the tyres let go: the sideways acceleration v2 / R is at most mu g.
inline double max_speed_in_turn(double mu, double radius) { return std::sqrt(mu * 9.81 * radius); }


}  // namespace vehicle
}  // namespace rc

#endif
