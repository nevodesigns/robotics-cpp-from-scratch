#ifndef STEPLIB_STEP_HPP
#define STEPLIB_STEP_HPP

namespace steplib {

// The rate limiter from lesson 01-02, as a library somebody else can install.
// The C++ here is finished. What this lesson is about is everything around it.
inline double step_toward(double current, double target, double max_step) {
  const double remaining = target - current;
  if (remaining <= max_step && -remaining <= max_step) return target;
  return current + (remaining > 0.0 ? max_step : -max_step);
}

}  // namespace steplib

#endif  // STEPLIB_STEP_HPP
