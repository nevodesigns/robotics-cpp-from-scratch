#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

// A first order model of a motor: the speed settles to `gain` times the command, 63 percent of the way in `tau` seconds.
struct Fit {
  bool ok = false;
  double gain = 0.0;
  double tau = 0.0;
  double rms = 0.0;     // how far the model is from the measurements, in the units of the speed
};

// The speed the model says there will be `t` seconds after a step of `command`.
inline double predict(double gain, double tau, double command, double t) {
  // TODO: gain * command * (1 - exp(-t / tau)). Zero if tau is not above zero or t is negative
  (void)gain;
  (void)tau;
  (void)command;
  (void)t;
  return 0.0;
}

// The root mean square of the difference between two traces, over as many points as both have. Zero if there are none.
inline double rms_error(const std::vector<double>& a, const std::vector<double>& b) {
  // TODO: over n points, n the smaller of the two sizes: the square root of the mean of (a[i] - b[i]) squared. Zero for no points
  (void)a;
  (void)b;
  return 0.0;
}

// Find the gain and time constant from a step response: `speeds[i]` was measured `times[i]` seconds after a step of `command` from rest.
//   gain: the speed it settled to, taken as the mean of the last tenth of the samples, over the command
//   tau:  if speed = final * (1 - exp(-t / tau)) then ln(1 - speed / final) = -t / tau, a straight line through the origin in t. Fit that line by least squares
//         over the samples that are between 5 and 90 percent of the way to the final speed, where the logarithm is well behaved (near the top it is the log of noise).
// Not ok if there are fewer than 10 samples, the traces differ in length, the command is zero, the final speed is zero, or no sample is in the range.
inline Fit fit_first_order(const std::vector<double>& times, const std::vector<double>& speeds, double command) {
  // TODO: Not ok (return the empty Fit) for fewer than 10 samples, traces of different lengths, or a zero command.
  // last = the mean of the last tenth of the speeds (at least one sample); a last of zero is not ok.
  // For each sample with fraction = speed / last between 0.05 and 0.90: y = log(1 - fraction); add t * t to stt and t * y to sty. No samples, or sty zero: not ok.
  // gain = last / command. tau = -stt / sty (least squares for y = -t / tau through the origin); a tau that is not above zero or not finite is not ok.
  // Then rms = rms_error of predict(gain, tau, command, t) against the speeds, and ok = true.
  (void)times;
  (void)speeds;
  (void)command;
  return Fit();
}

// Improve a fit by looking directly at how far the model is from the measurements. The straight line fit above is quick and is biased when the readings are noisy:
// taking the logarithm of a noisy number does not average to the logarithm of the number. This keeps the gain and searches for the time constant that gives the
// smallest rms error, between a third and three times the first estimate, by golden section search (40 steps). Not ok if the first fit was not.
inline Fit refine_first_order(const std::vector<double>& times, const std::vector<double>& speeds, double command) {
  // TODO: start = fit_first_order(...); not ok: return it. Keep its gain. error_at(tau) = rms_error of predict(gain, tau, command, t) against the speeds.
  // Golden section search for the tau that makes error_at smallest, between start.tau / 3 and start.tau * 3, 40 steps (phi = 0.618034): keep two interior points a and b; if error at a is less than at b the upper end becomes b, else the lower end becomes a, re-using the point that stays.
  // Return the start fit with tau at the middle of the final interval and rms recomputed.
  (void)times;
  (void)speeds;
  (void)command;
  return Fit();
}

#endif  // LESSON_SOLUTION_HPP
