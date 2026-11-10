#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

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
  // TODO: an empty response is diverged. Otherwise go through r.y once:
  //  - diverged is true if any value is not below 1e6 in size (write !(std::fabs(v) < 1e6) so a value
  //    that is not a number counts too);
  //  - the peak is the largest value;
  //  - the last moment more than 2 percent of the target away: when |v - target| > 0.02 * |target|,
  //    note (i + 1) * r.dt. The settling time is the last one noted, or 0 if it was never outside.
  // Then overshoot_percent = (peak - target) / target * 100 if the peak is above the target, else 0.
  // steady_error is |target - the mean of the last tenth of the samples| (from n - n / 10 to the end).
  (void)r;
  return Metrics();
}

// What the response is required to do.
struct Spec {
  double max_overshoot_percent = 10.0;
  double max_settling_time = 3.0;
  double max_steady_error = 0.01;
};

inline bool meets(const Spec& spec, const Metrics& m) {
  // TODO: false if diverged, or if overshoot_percent, settling_time or steady_error is above the
  // spec's maximum. Otherwise true.
  (void)spec;
  (void)m;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
