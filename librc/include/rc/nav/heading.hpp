// rc/nav/heading.hpp
//
// Angle arithmetic that does not turn the long way round, and a gyro and compass filter
// built on it, from lesson 16-08, graduated.
//
// An angle is a direction on a circle, and 3.1 and -3.1 radians are 0.083 apart across the
// back and not 6.2. Subtracting them gives 6.2, and a filter that pulls its estimate towards
// a compass by that difference turns the whole way round at every crossing. Measured on a
// robot turning for two minutes, nearly four turns, the worst error with the plain difference
// was 3.139 radians at a gain of 0.1, half a turn, and 0.162 with the short way round. The
// rms error was 0.048 against 1.461 for the gyro alone.
//
// The arithmetic mean of 3.1 and -3.1 is zero, which points the opposite way from both, so
// angles are averaged as unit vectors, and the result can be undefined when they cancel.

#ifndef RC_NAV_HEADING_HPP
#define RC_NAV_HEADING_HPP

#include <cmath>
#include <cstddef>

#include <rc/core/compat.hpp>

namespace rc {
namespace nav {

constexpr double kHeadingPi = 3.141592653589793;
constexpr double kHeadingTurn = 6.283185307179586;

// An angle brought into (-pi, pi].
//
// An angle is a direction, and 3.2 and -3.08 are the same one. Every comparison of
// two angles has to be made after they are brought to the same turn, and the simple
// ways to do it are wrong in different ways: a loop that subtracts a turn at a time
// never finishes for a big angle, and atan2 of the sine and cosine loses digits as the
// angle grows. This subtracts the whole number of turns at once.
inline double wrap_pi(double angle) {
  double wrapped = angle - kHeadingTurn * std::floor((angle + kHeadingPi) / kHeadingTurn);   // [-pi, pi)
  if (wrapped <= -kHeadingPi) wrapped = kHeadingPi;                                    // and -pi is pi
  return wrapped;
}

// How far round to turn to get from b to a, the short way, as a signed angle.
//
// 3.1 and -3.1 are 0.083 apart, across the back of the circle, and not 6.2 apart.
// Subtracting them gives 6.2, and any controller or filter that uses that number turns
// the whole way round.
inline double angle_difference(double a, double b) { return wrap_pi(a - b); }

// The average direction of some angles, optionally weighted.
//
// The arithmetic mean of 3.1 and -3.1 is zero, which points the opposite way from both
// of them. Angles are averaged as the directions they are: add the unit vectors and
// take the direction of the sum. If the vectors cancel, as they do for 0 and pi, there
// is no mean direction and `defined` says so.
inline double circular_mean(rc::span<const double> angles, rc::span<const double> weights, bool& defined) {
  double sx = 0.0;
  double sy = 0.0;
  for (std::size_t i = 0; i < angles.size(); ++i) {
    const double w = i < weights.size() ? weights[i] : 1.0;
    sx += w * std::cos(angles[i]);
    sy += w * std::sin(angles[i]);
  }

  defined = std::hypot(sx, sy) > 1e-12;
  if (!defined) return 0.0;
  return std::atan2(sy, sx);
}

// A heading estimate from a gyro and a compass.
//
// The gyro is smooth and drifts: it measures how fast the robot is turning, so its
// error accumulates. The compass is noisy and does not drift: it measures the heading
// directly, with scatter. Each covers the other's fault. The gyro carries the estimate
// between compass readings, and each reading pulls it a fraction of the way towards the
// truth, which cancels the drift without passing on the noise.
//
// That fraction is the gain. Small trusts the gyro and lets its drift in. Large trusts
// the compass and lets its noise in.
class HeadingFilter {
 public:
  HeadingFilter(double gain, double initial = 0.0) : gain_(gain), heading_(wrap_pi(initial)) {}

  // Integrates the gyro over dt, and keeps the heading in range, so that it does not
  // grow without limit as the robot turns round.
  void predict(double rate, double dt) { heading_ = wrap_pi(heading_ + rate * dt); }

  // Pulls the estimate towards a compass reading, along the SHORT way round.
  void correct(double compass) { heading_ = wrap_pi(heading_ + gain_ * angle_difference(compass, heading_)); }

  double heading() const { return heading_; }

 private:
  double gain_;
  double heading_;
};

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_HEADING_HPP
