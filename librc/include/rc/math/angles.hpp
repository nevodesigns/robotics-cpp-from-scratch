// rc/math/angles.hpp
//
// Angle handling from lesson 01-03, graduated.
//
// An angle is not a number, and the difference shows up the first time two of
// them are subtracted. A heading of 179 degrees and one of minus 179 are two
// degrees apart, and the arithmetic difference says 358, which is the long way
// round and is the direction a robot will actually turn if nobody wrapped it.

#ifndef RC_MATH_ANGLES_HPP
#define RC_MATH_ANGLES_HPP

#include <cmath>

namespace rc {
namespace math {

constexpr double kPi = 3.14159265358979323846;

inline double wrap_angle(double radians) {
  const double full_turn = 2.0 * kPi;

  // fmod leaves the result in minus full_turn to plus full_turn, keeping the
  // sign of the input. That is halfway there.
  double wrapped = std::fmod(radians + kPi, full_turn);

  // fmod(-1.0, 6.28) is -1.0, not 5.28, so a negative result needs one more
  // turn added before shifting back. Forgetting this is the whole bug.
  if (wrapped < 0.0) wrapped += full_turn;

  return wrapped - kPi;
}

inline double shortest_turn(double from, double to) {
  // Subtract first, then wrap. Wrapping the difference is what turns 358
  // degrees into minus 2 degrees.
  return wrap_angle(to - from);
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_ANGLES_HPP
