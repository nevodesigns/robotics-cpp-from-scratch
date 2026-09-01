#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// The thing under test, behind an interface so one set of checks can be pointed
// at more than one implementation. This is the rate limiter from lesson 01-02:
// move `current` toward `target`, by no more than `max_step`.
class Limiter {
 public:
  virtual ~Limiter() = default;
  virtual double apply(double current, double target, double max_step) const = 0;
};

// Everything a rate limiter must do, whatever is inside it.
//
// Written as properties rather than as remembered answers, because a property
// is something you can write before any implementation exists, and because a
// list of remembered answers only ever catches the mistakes you already thought
// of.
//
// Each check below is here because an implementation exists that passes every
// other one and fails this. That is the standard worth holding a test suite to:
// not "does it pass" but "what would have to be wrong for this to fail".
inline bool checks_pass(const Limiter& limiter) {
  const double tolerance = 1e-9;

  // It arrives. Within one step of the target, it lands on it rather than
  // stepping past and coming back, which is the oscillation of E-NUM-0005.
  if (std::fabs(limiter.apply(0.95, 1.0, 0.1) - 1.0) > tolerance) return false;
  if (std::fabs(limiter.apply(1.05, 1.0, 0.1) - 1.0) > tolerance) return false;

  // It limits. A target far away is approached by exactly one step, not
  // reached.
  if (std::fabs(limiter.apply(0.0, 100.0, 0.1) - 0.1) > tolerance) return false;

  // It goes the right way. Downward is a direction too, and a limiter that only
  // ever adds passes every test written with the target above the current
  // value.
  if (std::fabs(limiter.apply(0.0, -100.0, 0.1) - -0.1) > tolerance) return false;

  // It stays put when it has arrived. Equality is a boundary, and a boundary is
  // where an implementation stops being described by the case either side of it.
  if (std::fabs(limiter.apply(1.0, 1.0, 0.1) - 1.0) > tolerance) return false;

  // A step of nothing moves nothing. This is the case a caller reaches by
  // passing a variable that happened to be zero, rather than on purpose.
  if (std::fabs(limiter.apply(0.0, 100.0, 0.0) - 0.0) > tolerance) return false;

  // Exactly one step away is the boundary between arriving and stepping, and it
  // is chosen deliberately because sampling at random almost never lands on it.
  if (std::fabs(limiter.apply(0.0, 0.1, 0.1) - 0.1) > tolerance) return false;
  if (std::fabs(limiter.apply(0.0, -0.1, 0.1) - -0.1) > tolerance) return false;

  // It never goes further than it was asked to, anywhere in the range. A loop
  // is the cheap way to say "and not just at the points I thought of".
  for (int i = -50; i <= 50; ++i) {
    const double current = static_cast<double>(i) * 0.37;
    const double moved = limiter.apply(current, 12.5, 0.25);
    if (std::fabs(moved - current) > 0.25 + tolerance) return false;

    // And it never ends up further from the target than it started.
    if (std::fabs(12.5 - moved) > std::fabs(12.5 - current) + tolerance) return false;
  }

  return true;
}

#endif  // LESSON_SOLUTION_HPP
