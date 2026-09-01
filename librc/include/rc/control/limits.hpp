// rc/control/limits.hpp
//
// Clamping and rate limiting from lesson 01-02, graduated.
//
// The rate limiter is the one with the trap in it: always moving by the maximum
// step overshoots the target and comes back, for ever, which on real hardware
// is a visibly vibrating actuator. Comparing the remaining distance against the
// step first is what makes it land.

#ifndef RC_CONTROL_LIMITS
#define RC_CONTROL_LIMITS

#include <cmath>

namespace rc {
namespace control {

inline double clamp(double value, double low, double high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

inline double rate_limit(double current, double target, double max_step) {
  const double change = target - current;

  // Take the whole remaining change when it is small enough. Without this the
  // value steps past the target and then steps back, forever, which looks like
  // a vibrating motor and is a real failure people ship.
  if (change > max_step) return current + max_step;
  if (change < -max_step) return current - max_step;
  return target;
}

inline int steps_to_reach(double current, double target, double max_step) {
  // Guard the case that never terminates, before entering a loop that would
  // otherwise run forever. A loop whose exit depends on caller supplied numbers
  // needs this check every time.
  if (max_step <= 0.0) return std::fabs(target - current) <= 1e-9 ? 0 : -1;

  // Comparing two computed doubles with != is the trap lesson 00-04 warns about.
  // Adding 0.1 ten times does not produce exactly 1.0, so an exact comparison
  // would report eleven steps for a journey that plainly takes ten. Asking
  // whether the remaining distance is small enough is the correct question.
  const double arrived = 1e-9;

  int steps = 0;
  while (std::fabs(target - current) > arrived) {
    current = rate_limit(current, target, max_step);
    ++steps;

    // A second belt to go with the braces: even with the guard above, refusing
    // to loop without bound is cheap insurance in code that drives hardware.
    if (steps > 1000000) return -1;
  }
  return steps;
}

}  // namespace control
}  // namespace rc

#endif  // RC_CONTROL_LIMITS
