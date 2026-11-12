#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <rc/field/terrain.hpp>

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


#endif
