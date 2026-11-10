// rc/test/response.hpp
//
// Asserting a behaviour and not a number, from lesson 05-08, graduated.
//
// A test that pins the value of a response at one second broke for all 6 of 6 harmless changes (a time step halved or doubled, a plant 5 or 10 percent slower or
// faster): the value moved in the third or fourth decimal place. A test of overshoot, settling time and steady error passed all 6 harmless changes and failed all 5 real bugs
// (the integral lost, gains too low, the integral too strong with 33.56 percent overshoot, gains too high with 18.29, and the sign flipped, which runs away). A specification can still be written
// too tight: with the settling time cut to 1.8 s, 4 of the 6 harmless changes failed it, which is the pinned value again.

#ifndef RC_TEST_RESPONSE_HPP
#define RC_TEST_RESPONSE_HPP

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace test {

// A step response: the output at each moment after the target jumped from zero to `target`.
struct Response {
  std::vector<double> y;
  double dt = 0.001;
  double target = 1.0;
};

// A first-order plant with time constant tau, driven by a PI controller. Given.
//
// y moves towards the controller's output: dy/dt = (u - y) / tau. The controller output is kp times the error
// plus ki times the error added up over time.
inline Response simulate(double kp, double ki, double tau, double dt, double seconds) {
  Response r;
  r.dt = dt;
  double y = 0.0;
  double integral = 0.0;
  const int steps = static_cast<int>(seconds / dt);
  for (int i = 0; i < steps; ++i) {
    const double error = r.target - y;
    integral += error * dt;
    const double u = kp * error + ki * integral;
    y += dt * (u - y) / tau;
    r.y.push_back(y);
  }
  return r;
}

// What a person asks of a response, as numbers.
struct Metrics {
  double overshoot_percent = 0.0;   // how far past the target it went, as a percent of the target
  double settling_time = 0.0;       // the last moment it was more than 2 percent from the target
  double steady_error = 0.0;        // how far from the target it ends up
  bool diverged = false;            // it ran away
};

inline Metrics measure(const Response& r) {
  Metrics m;
  const std::size_t n = r.y.size();
  if (n == 0) {
    m.diverged = true;
    return m;
  }

  double peak = r.y[0];
  double last_outside = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    const double v = r.y[i];
    if (!(std::fabs(v) < 1e6)) m.diverged = true;     // also catches a value that is not a number
    if (v > peak) peak = v;
    if (std::fabs(v - r.target) > 0.02 * std::fabs(r.target)) last_outside = static_cast<double>(i + 1) * r.dt;
  }

  m.overshoot_percent = peak > r.target ? (peak - r.target) / r.target * 100.0 : 0.0;
  m.settling_time = last_outside;

  // The error at the end: the average of the last tenth of the run, so one noisy sample does not decide it.
  const std::size_t from = n - n / 10;
  double sum = 0.0;
  for (std::size_t i = from; i < n; ++i) sum += r.y[i];
  m.steady_error = std::fabs(r.target - sum / static_cast<double>(n - from));
  return m;
}

// What the response is required to do.
struct Spec {
  double max_overshoot_percent = 10.0;
  double max_settling_time = 3.0;
  double max_steady_error = 0.01;
};

inline bool meets(const Spec& spec, const Metrics& m) {
  if (m.diverged) return false;
  if (m.overshoot_percent > spec.max_overshoot_percent) return false;
  if (m.settling_time > spec.max_settling_time) return false;
  if (m.steady_error > spec.max_steady_error) return false;
  return true;
}

}  // namespace test
}  // namespace rc

#endif  // RC_TEST_RESPONSE_HPP
