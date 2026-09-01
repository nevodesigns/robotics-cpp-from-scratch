// The corrected file.
//
// Read this after your own version compiles, not before. The four fixes are
// marked, and the comment beside each says what the compiler said and how to
// get from that message to the change.

#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// Fix one, and the one that mattered most.
//
//   error: 'sqrt' is not a member of 'std'
//   note: 'std::sqrt' is defined in header '<cmath>'; did you forget to
//         '#include <cmath>'?
//
// Three of the six errors said this, because three functions below use a name
// from this header. One line removed all three. The note is the compiler
// telling you the answer, and it is easy to skip because it does not say
// "error".
#include <cmath>

// How far a point is from the origin, by Pythagoras.
inline double distance_from_origin(double x, double y) {
  return std::sqrt(x * x + y * y);
}

// How fast something is moving, given its speed along each axis.
inline double speed_from_components(double vx, double vy) {
  return std::sqrt(vx * vx + vy * vy);
}

// Whether a value is close enough to zero to treat as zero.
inline bool near_zero(double value) {
  return std::fabs(value) < 1e-9;
}

// Forces a percentage into the range 0 to 100.
inline int clamp_percent(int value) {
  // Fix two.
  //
  //   error: expected ';' before 'if'
  //
  // The compiler read the next line as a continuation of this statement and
  // stopped at the first token that could not follow. The caret sat at the end
  // of this line, which is where the semicolon belongs.
  if (value < 0) return 0;
  if (value > 100) return 100;
  return value;
}

// Converts a battery reading in millivolts to a percentage.
inline int battery_percent(int millivolts) {
  const int empty_mv = 3000;
  const int full_mv = 4200;

  // Fix three.
  //
  //   error: 'milivolts' was not declared in this scope;
  //          did you mean 'millivolts'?
  //
  // A name with one letter missing is a different name, and the compiler has
  // no way to know it was meant to be the parameter. It guessed anyway, which
  // is worth reading before assuming a suggestion is wrong.
  if (millivolts <= empty_mv) return 0;
  if (millivolts >= full_mv) return 100;

  return ((millivolts - empty_mv) * 100) / (full_mv - empty_mv);
}

// Whether a point is more than ten metres from the origin.
inline bool is_far(double x, double y) {
  // Fix four.
  //
  //   error: too few arguments to function
  //          'double distance_from_origin(double, double)'
  //   note: declared here
  //
  // The note points at the declaration, which is the fastest way to see what
  // the function actually wanted. Two coordinates, and only one was passed.
  return distance_from_origin(x, y) > 10.0;
}

#endif  // LESSON_SOLUTION_HPP
