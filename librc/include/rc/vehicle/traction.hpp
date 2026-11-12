// rc/vehicle/traction.hpp
//
// The traction circle, from lesson 23-05.
//
// A tyre has one budget of grip, mu g cos(alpha) on a slope of angle alpha, to spend on turning, on speeding up and on braking, and the sum of the squares of the sideways acceleration and the longitudinal acceleration cannot exceed the square of the budget. traction_budget takes a terrain cell (lesson 22-05), a heading, a speed and a curvature, and gives the sideways acceleration the turn needs, the share of the grip it takes, and the most the vehicle can speed up and the most it can brake in that turn, with the slope and the rolling resistance; slides says that the turn alone is more than the grip. max_speed_for_curvature and max_curvature_at_speed are the same limit solved for the other variable.
//
// On gravel at 1.5 m/s round a circle of 0.742 m, the turn took 69 percent of the grip, the most the vehicle could speed up fell from 4.02 to 2.82 m/s2, the most it could brake from 4.81 to 3.60 m/s2 (the stop from 1.5 m/s went from 0.23 to 0.31 m), at 1.81 m/s the turn alone used all of the grip, and at 99 percent of it the vehicle had 20 percent of its straight line braking left.

#ifndef RC_RC_VEHICLE_TRACTION_HPP
#define RC_RC_VEHICLE_TRACTION_HPP

#include <algorithm>
#include <cmath>
#include <rc/field/terrain.hpp>

namespace rc {
namespace vehicle {

// What the grip allows at one moment: the sideways acceleration the turn needs, the share of the grip it takes, and what is left to speed up or to brake with.
struct TractionBudget {
  double lateral = 0.0;   // m/s2 the turn needs: speed squared times curvature
  double share = 0.0;     // lateral / (mu g cos(alpha)): 1 or more means the tyres let go
  double drive = 0.0;     // the most it can speed up in this turn, m/s2 (may be negative: it cannot hold the speed)
  double brake = 0.0;     // the most it can slow down in this turn, m/s2
  bool slides = false;
};

// The budget for a vehicle at a speed on a curve of a given curvature (1 / radius, positive either way), heading a given way over a terrain cell.
inline TractionBudget traction_budget(const rc::field::Cell& cell, double heading, double speed, double curvature) {
  TractionBudget b;
  const double g = 9.81;
  const rc::field::Traction t = rc::field::traction(cell, heading);
  const double ca = std::cos(t.angle);
  const double sa = std::sin(t.angle);
  const double grip = cell.surface.mu * g * ca;
  b.lateral = speed * speed * std::fabs(curvature);
  b.share = grip > 0.0 ? b.lateral / grip : 1.0e9;
  b.slides = b.share >= 1.0;
  const double left = b.slides ? 0.0 : grip * std::sqrt(1.0 - b.share * b.share);
  const double rolling = cell.surface.crr * g * ca;
  b.drive = left - g * sa - rolling;
  b.brake = left + g * sa + rolling;
  return b;
}

// The fastest speed on a curve before the tyres let go, on the surface and the slope of a cell.
inline double max_speed_for_curvature(const rc::field::Cell& cell, double heading, double curvature) {
  const rc::field::Traction t = rc::field::traction(cell, heading);
  const double grip = cell.surface.mu * 9.81 * std::cos(t.angle);
  return std::fabs(curvature) > 1e-12 ? std::sqrt(grip / std::fabs(curvature)) : 1.0e9;
}

// The tightest curve at a speed.
inline double max_curvature_at_speed(const rc::field::Cell& cell, double heading, double speed) {
  const rc::field::Traction t = rc::field::traction(cell, heading);
  const double grip = cell.surface.mu * 9.81 * std::cos(t.angle);
  return speed > 1e-12 ? grip / (speed * speed) : 1.0e9;
}


}  // namespace vehicle
}  // namespace rc

#endif
