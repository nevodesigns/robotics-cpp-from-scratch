#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <rc/core/clock.hpp>

// A reading, and the moment it describes. Not the moment it arrived.
template <class T>
struct Stamped {
  T value{};
  rc::core::Nanoseconds sampled_at = 0;
  bool valid = false;
};

// TODO 1: how old this reading is at `now`, in seconds.
//
// rc::core::seconds_between(from, to) does the conversion, and it is the only
// place in this curriculum where nanoseconds become a double, deliberately.
//
// Do not clamp a negative result. A reading stamped in the future means two
// clocks disagree, which is worth seeing rather than hiding.
template <class T>
double age_seconds(const Stamped<T>& reading, rc::core::Nanoseconds now) {
  (void)reading;
  (void)now;
  return 0.0;
}

// TODO 2: whether a reading is recent enough to act on.
//
// False if it never arrived. False if it is older than max_age_seconds. False
// if its age is negative.
//
// Write the condition as the requirement and negate it, rather than as a list
// of things to reject. A nan max_age must fail this, and it only does if the
// test is phrased that way round: every comparison against a nan is false, so
// "age <= max_age" is false and the reading is refused, while "age > max_age"
// is also false and the reading is accepted. That is E-SENSE-0007.
template <class T>
bool fresh(const Stamped<T>& reading, rc::core::Nanoseconds now, double max_age_seconds) {
  (void)reading;
  (void)now;
  (void)max_age_seconds;
  return true;
}

// TODO 3: the same reading, carried forward to `now` at a rate you believe.
//
// value + rate * age. Stamp the result at `now`, because that is the moment it
// now describes. An invalid reading comes back unchanged: there is nothing to
// carry forward, and inventing a value for it would be worse than admitting it.
inline Stamped<double> carried_forward(const Stamped<double>& reading, double rate,
                                       rc::core::Nanoseconds now) {
  (void)rate;
  (void)now;
  return reading;
}

#endif  // LESSON_SOLUTION_HPP
