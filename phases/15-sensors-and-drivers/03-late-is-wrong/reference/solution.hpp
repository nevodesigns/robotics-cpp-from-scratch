#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <rc/core/clock.hpp>

// A reading, and the moment it describes.
//
// Not the moment it arrived. Those are different times and the gap between them
// is never zero: the device samples, converts, buffers, waits for its turn on
// the bus, and is finally copied into your process, and none of that shows up
// anywhere in your code. A reading with no time attached is silently claimed to
// be about now, and it never is.
//
// The whole error is speed times latency. Standing still that is zero, which is
// exactly why this survives every test on a bench.
template <class T>
struct Stamped {
  T value{};
  rc::core::Nanoseconds sampled_at = 0;
  bool valid = false;
};

// How old this reading is, in seconds.
//
// Negative means it is stamped in the future, which is not a strange edge case:
// it is two clocks disagreeing, and it happens the moment a timestamp comes
// from a device with its own oscillator. Report it rather than clamping it away.
template <class T>
double age_seconds(const Stamped<T>& reading, rc::core::Nanoseconds now) {
  return rc::core::seconds_between(reading.sampled_at, now);
}

// Whether a reading is recent enough to act on.
//
// Written as the requirement, then negated. An age that is negative fails it, a
// reading that never arrived fails it, and a nan max_age fails it, because
// every comparison against a nan is false and this phrasing turns that into a
// refusal rather than an acceptance. That is E-SENSE-0007 from the last lesson,
// and it is the reason every range check in this curriculum reads this way.
template <class T>
bool fresh(const Stamped<T>& reading, rc::core::Nanoseconds now, double max_age_seconds) {
  if (!reading.valid) return false;
  const double age = age_seconds(reading, now);
  return age >= 0.0 && age <= max_age_seconds;
}

// The same reading, carried forward to now at a rate you believe.
//
// This is the cheap fix and it works: at 1 m/s with 20 ms of latency it removes
// 2 cm of error, and what is left is 2 cm times however wrong the rate was. A
// rate that is 10 percent out still leaves you nine tenths better off.
//
// It cannot be done at all without the timestamp, which is the point of
// carrying one.
inline Stamped<double> carried_forward(const Stamped<double>& reading, double rate,
                                       rc::core::Nanoseconds now) {
  if (!reading.valid) return reading;

  Stamped<double> moved = reading;
  moved.value = reading.value + rate * age_seconds(reading, now);
  moved.sampled_at = now;
  return moved;
}

#endif  // LESSON_SOLUTION_HPP
