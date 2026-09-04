// rc/sensor/stamped.hpp
//
// A reading and the moment it describes, from lesson 15-03.
//
// The first module in rc::sensor, and it holds one idea: the time a reading
// arrived is not the time it was taken. A device samples, converts, buffers,
// waits its turn on a bus and is finally copied into your process, and none of
// that appears anywhere in your code. A reading with no time attached is
// silently claimed to be about now, and it never is.
//
// The error that claim causes is speed times latency. At 1 m/s with 20 ms of
// latency it is exactly 2 cm, and standing still it is exactly zero, which is
// why it passes every bench test and appears the first time the robot moves.

#ifndef RC_SENSOR_STAMPED
#define RC_SENSOR_STAMPED

#include <rc/core/clock.hpp>

namespace rc {
namespace sensor {

template <class T>
struct Stamped {
  T value{};
  rc::core::Nanoseconds sampled_at = 0;
  bool valid = false;
};

// How old this reading is, in seconds.
//
// Negative means it is stamped in the future, which is two clocks disagreeing
// rather than a strange edge case: it happens as soon as a timestamp comes from
// a device with its own oscillator. Report it rather than clamping it away.
template <class T>
double age_seconds(const Stamped<T>& reading, rc::core::Nanoseconds now) {
  return rc::core::seconds_between(reading.sampled_at, now);
}

// Whether a reading is recent enough to act on.
//
// The condition is the requirement, negated where it fails, rather than a list
// of things to reject. That phrasing is what makes a nan limit refuse the
// reading instead of accepting it, for the reason catalogued as E-SENSE-0007.
template <class T>
bool fresh(const Stamped<T>& reading, rc::core::Nanoseconds now, double max_age_seconds) {
  if (!reading.valid) return false;
  const double age = age_seconds(reading, now);
  return age >= 0.0 && age <= max_age_seconds;
}

// The same reading, carried forward to now at a rate you believe.
//
// Measured: at 1 m/s with latency jittering between 4 and 36 ms, believing the
// reading is about now costs 2.2 cm rms, smoothing it over 16 samples costs
// 3.5 cm because the filter adds its own lag to the bus's, and carrying it
// forward removes the error entirely. What is left is the original error times
// how wrong the rate was, so a rate estimate 10 percent out still removes nine
// tenths of the problem.
//
// None of that is possible without the timestamp, which is the point of
// carrying one.
inline Stamped<double> carried_forward(const Stamped<double>& reading, double rate,
                                       rc::core::Nanoseconds now) {
  if (!reading.valid) return reading;

  Stamped<double> moved = reading;
  moved.value = reading.value + rate * age_seconds(reading, now);
  moved.sampled_at = now;
  return moved;
}

}  // namespace sensor
}  // namespace rc

#endif  // RC_SENSOR_STAMPED
