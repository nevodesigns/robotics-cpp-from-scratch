#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// Forces value into the range low to high.
inline double clamp(double value, double low, double high) {
  // TODO: two comparisons is all this needs.
  (void)low;
  (void)high;
  return value;
}

// Moves current towards target by at most max_step.
//
// Returns the new value. If the remaining distance is smaller than max_step,
// return target exactly rather than overshooting it.
inline double rate_limit(double current, double target, double max_step) {
  // TODO
  (void)target;
  (void)max_step;
  return current;
}

// How many rate limited steps it takes to get from current to target.
// Returns 0 when already there, and -1 when max_step is zero or negative,
// because that case never arrives.
inline int steps_to_reach(double current, double target, double max_step) {
  // TODO: a loop, with a guard before it.
  // Careful: do not compare the two doubles with !=. Ask whether the remaining
  // distance is small enough instead, for the reason lesson 00-03 gave you.
  (void)current;
  (void)target;
  (void)max_step;
  return 0;
}

#endif  // LESSON_SOLUTION_HPP
