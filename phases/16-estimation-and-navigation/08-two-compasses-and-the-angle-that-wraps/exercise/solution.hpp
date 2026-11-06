#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>

#include <rc/core/compat.hpp>

constexpr double kPi = 3.141592653589793;
constexpr double kTurn = 6.283185307179586;

// An angle brought into (-pi, pi].
inline double wrap_pi(double angle) {
  // TODO: subtract the whole number of turns at once: angle - kTurn * floor((angle + kPi) / kTurn)
  // lands in [-pi, pi). The range should be (-pi, pi], so a result of exactly -pi becomes pi.
  //
  // Not a loop that takes off a turn at a time, which never finishes for a big angle,
  // and not atan2(sin, cos), which loses digits as the angle grows.
  (void)angle;
  return 0.0;
}

// How far round to turn to get from b to a, the short way, as a signed angle. 3.1 and
// -3.1 are 0.083 apart, across the back of the circle, and not 6.2.
inline double angle_difference(double a, double b) {
  // TODO
  (void)a;
  (void)b;
  return 0.0;
}

// The average direction of some angles, optionally weighted. `defined` is false when
// there is none.
inline double circular_mean(rc::span<const double> angles, rc::span<const double> weights, bool& defined) {
  // TODO: the arithmetic mean of 3.1 and -3.1 is zero, which points the opposite way from
  // both. Add the unit vectors instead, (cos a, sin a) times the weight, and take the
  // direction of the sum with atan2. A missing weight is one.
  //
  // If the vectors cancel, as they do for 0 and pi, the length of the sum is about zero
  // and there is no mean direction: set defined to false (the length is at most 1e-12)
  // and return zero.
  (void)angles;
  (void)weights;
  defined = false;
  return 0.0;
}

// A heading estimate from a gyro and a compass.
class HeadingFilter {
 public:
  HeadingFilter(double gain, double initial = 0.0) : gain_(gain), heading_(wrap_pi(initial)) {}

  // Integrates the gyro over dt, and keeps the heading in range.
  void predict(double rate, double dt) {
    // TODO: heading plus rate times dt, wrapped.
    (void)rate;
    (void)dt;
    (void)gain_;
  }

  // Pulls the estimate towards a compass reading, along the SHORT way round: by the
  // gain times the angle difference, wrapped.
  void correct(double compass) {
    // TODO
    (void)compass;
  }

  double heading() const { return heading_; }

 private:
  double gain_;
  double heading_;
};

#endif  // LESSON_SOLUTION_HPP
