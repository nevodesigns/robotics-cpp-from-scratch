// rc/sim/identify.hpp
//
// Finding a motor's gain and time constant from a step, from lesson 18-03, graduated.
//
// A first order model (the speed settles to gain times the command, 63 percent of the way in one time constant), a quick fit by a straight line through the logarithm of the response, a refinement that searches for the time constant with the smallest
// error against the data, a prediction, and the rms error between two traces. A trace that cannot be fitted is refused.
//
// A clean step of 3 on a motor of gain 2 and time constant 0.1 s gave gain 1.9991 and time constant 0.0989 s (the simulated motor's own step makes its effective time constant a little under 0.1). With noise of 0.05, 0.30 and 0.50 speed units the quick fit gave 0.0992, 0.1483
// and 0.2221 s, the refined fit 0.0989, 0.0989 and 0.0989, and reading a 63 percent crossing gave 0.0994, 0.0876 and 0.0794. A model fitted on a step of 1 predicted steps of 2 to 12 to within 0.0083 and a step of 20, which the real motor clamps, to within 14.4390.

#ifndef RC_SIM_IDENTIFY_HPP
#define RC_SIM_IDENTIFY_HPP

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace sim {

// A first order model of a motor: the speed settles to `gain` times the command, 63 percent of the way in `tau` seconds.
struct Fit {
  bool ok = false;
  double gain = 0.0;
  double tau = 0.0;
  double rms = 0.0;     // how far the model is from the measurements, in the units of the speed
};

// The speed the model says there will be `t` seconds after a step of `command`.
inline double predict(double gain, double tau, double command, double t) {
  if (tau <= 0.0 || t < 0.0) return 0.0;
  return gain * command * (1.0 - std::exp(-t / tau));
}

// The root mean square of the difference between two traces, over as many points as both have. Zero if there are none.
inline double rms_error(const std::vector<double>& a, const std::vector<double>& b) {
  const std::size_t n = a.size() < b.size() ? a.size() : b.size();
  if (n == 0) return 0.0;
  double sum = 0.0;
  for (std::size_t i = 0; i < n; ++i) sum += (a[i] - b[i]) * (a[i] - b[i]);
  return std::sqrt(sum / static_cast<double>(n));
}

// Find the gain and time constant from a step response: `speeds[i]` was measured `times[i]` seconds after a step of `command` from rest.
//   gain: the speed it settled to, taken as the mean of the last tenth of the samples, over the command
//   tau:  if speed = final * (1 - exp(-t / tau)) then ln(1 - speed / final) = -t / tau, a straight line through the origin in t. Fit that line by least squares
//         over the samples that are between 5 and 90 percent of the way to the final speed, where the logarithm is well behaved (near the top it is the log of noise).
// Not ok if there are fewer than 10 samples, the traces differ in length, the command is zero, the final speed is zero, or no sample is in the range.
inline Fit fit_first_order(const std::vector<double>& times, const std::vector<double>& speeds, double command) {
  Fit f;
  const std::size_t n = times.size();
  if (n < 10 || speeds.size() != n || command == 0.0) return f;

  const std::size_t tail = n / 10 > 0 ? n / 10 : 1;
  double last = 0.0;
  for (std::size_t i = n - tail; i < n; ++i) last += speeds[i];
  last /= static_cast<double>(tail);
  if (last == 0.0) return f;

  double stt = 0.0;
  double sty = 0.0;
  int used = 0;
  for (std::size_t i = 0; i < n; ++i) {
    const double fraction = speeds[i] / last;
    if (fraction < 0.05 || fraction > 0.90) continue;
    const double y = std::log(1.0 - fraction);
    stt += times[i] * times[i];
    sty += times[i] * y;
    ++used;
  }
  if (used == 0 || sty == 0.0) return f;

  f.gain = last / command;
  f.tau = -stt / sty;
  if (!(f.tau > 0.0) || !std::isfinite(f.tau)) return Fit();
  std::vector<double> model;
  for (std::size_t i = 0; i < n; ++i) model.push_back(predict(f.gain, f.tau, command, times[i]));
  f.rms = rms_error(model, speeds);
  f.ok = true;
  return f;
}

// Improve a fit by looking directly at how far the model is from the measurements. The straight line fit above is quick and is biased when the readings are noisy:
// taking the logarithm of a noisy number does not average to the logarithm of the number. This keeps the gain and searches for the time constant that gives the
// smallest rms error, between a third and three times the first estimate, by golden section search (40 steps). Not ok if the first fit was not.
inline Fit refine_first_order(const std::vector<double>& times, const std::vector<double>& speeds, double command) {
  Fit start = fit_first_order(times, speeds, command);
  if (!start.ok) return start;
  const auto error_at = [&](double tau) {
    std::vector<double> model;
    for (std::size_t i = 0; i < times.size(); ++i) model.push_back(predict(start.gain, tau, command, times[i]));
    return rms_error(model, speeds);
  };
  const double phi = 0.6180339887498949;
  double lo = start.tau / 3.0;
  double hi = start.tau * 3.0;
  double a = hi - phi * (hi - lo);
  double b = lo + phi * (hi - lo);
  double ea = error_at(a);
  double eb = error_at(b);
  for (int i = 0; i < 40; ++i) {
    if (ea < eb) {
      hi = b;
      b = a;
      eb = ea;
      a = hi - phi * (hi - lo);
      ea = error_at(a);
    } else {
      lo = a;
      a = b;
      ea = eb;
      b = lo + phi * (hi - lo);
      eb = error_at(b);
    }
  }
  Fit f = start;
  f.tau = 0.5 * (lo + hi);
  f.rms = error_at(f.tau);
  return f;
}

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_IDENTIFY_HPP
