#include <rc/test/rc_test.hpp>
#include <rc/sim/plant.hpp>
#include <rc/sim/sensor_model.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// One run of a position loop to a target of 1, read every 10 ms for 2 s, on a motor with the given time constant, through a sensor with the given noise and delay.
std::vector<double> run(double tau, double gain, double noise, int delay, std::uint32_t seed) {
  rc::sim::SimMotor motor(gain, tau, 12.0);
  rc::sim::SensorOptions so;
  so.noise = noise;
  so.delay = delay;
  so.seed = seed;
  rc::sim::SensorModel sensor(so);
  std::vector<double> trace;
  double measured = 0.0;
  for (int i = 0; i < 200; ++i) {
    motor.apply(6.0 * (1.0 - measured), 0.01);
    measured = sensor.read(motor.position()).value;
    trace.push_back(measured);
  }
  return trace;
}

// The motor's response to a command held at 3, read through a sensor: an open loop run, so that a delay in the sensor shifts the trace and changes nothing else.
std::vector<double> open_loop(double tau, double noise, int delay, std::uint32_t seed) {
  rc::sim::SimMotor motor(2.0, tau, 12.0);
  rc::sim::SensorOptions so;
  so.noise = noise;
  so.delay = delay;
  so.seed = seed;
  rc::sim::SensorModel sensor(so);
  std::vector<double> trace;
  for (int i = 0; i < 200; ++i) {
    motor.apply(3.0, 0.01);
    trace.push_back(sensor.read(motor.velocity()).value);
  }
  return trace;
}

std::string joined(const std::vector<std::string>& v) {
  std::string s;
  for (const std::string& x : v) s += (s.empty() ? "" : " ") + x;
  return s.empty() ? "none" : s;
}

}  // namespace

RC_TEST("a trace compared with itself has no error, no bias and no lag") {
  const std::vector<double> t = run(0.1, 2.0, 0.0, 0, 1);
  const Metrics m = compare(t, t, 10);
  RC_CHECK(t.size() == 200);
  RC_CHECK_NEAR(m.rmse, 0.0, 1e-12);
  RC_CHECK_NEAR(m.max_abs, 0.0, 1e-12);
  RC_CHECK_NEAR(m.bias, 0.0, 1e-12);
  RC_CHECK(m.lag == 0);
  RC_CHECK_NEAR(m.rmse_at_lag, 0.0, 1e-12);
  std::vector<double> one_off = t;
  one_off[10] += 1.0;                                                              // a single sample is wrong, by a whole unit
  const Metrics blip = compare(one_off, t, 5);
  RC_CHECK_NEAR(blip.max_abs, 1.0, 1e-12);
  RC_CHECK_NEAR(blip.rmse, std::sqrt(1.0 / 200.0), 1e-12);
  const Metrics none = compare(std::vector<double>(), t, 5);
  RC_CHECK_NEAR(none.rmse, 0.0, 1e-12);
  RC_CHECK(none.lag == 0);
}

RC_TEST("a simulation that sits above the real trace by a constant has that bias and that error and no lag") {
  const std::vector<double> real = run(0.1, 2.0, 0.0, 0, 1);
  std::vector<double> sim = real;
  for (double& v : sim) v += 0.05;
  const Metrics m = compare(sim, real, 10);
  std::cout << "  a trace 0.05 too high everywhere: rmse " << std::fixed << std::setprecision(4) << m.rmse << ", largest " << m.max_abs << ", bias " << m.bias << ", lag " << m.lag << "\n\n";
  RC_CHECK_NEAR(m.rmse, 0.05, 1e-9);
  RC_CHECK_NEAR(m.max_abs, 0.05, 1e-9);
  RC_CHECK_NEAR(m.bias, 0.05, 1e-9);
  RC_CHECK(m.lag == 0);
}

RC_TEST("a sensor that is late shows as a lag, and the error at that lag is far below the error with the traces lined up sample for sample") {
  const std::vector<double> sim = open_loop(0.1, 0.0, 0, 1);
  const std::vector<double> real = open_loop(0.1, 0.0, 5, 1);                       // the real sensor is 5 samples (50 ms) late
  const Metrics m = compare(sim, real, 20);
  std::cout << "  a motor's speed read through a sensor 5 samples late (open loop): error lined up " << std::fixed << std::setprecision(4) << m.rmse << ", best shift " << m.lag
            << " samples, error at that shift " << m.rmse_at_lag << "\n";
  RC_CHECK(m.lag == 5);
  RC_CHECK(m.rmse > 0.1);
  RC_CHECK(m.rmse_at_lag < 0.1 * m.rmse);
  const Metrics reverse = compare(real, sim, 20);
  RC_CHECK(reverse.lag == -m.lag);                                                 // the sign says which is late
  std::vector<double> shifted(sim.size());
  for (std::size_t i = 0; i < sim.size(); ++i) shifted[i] = sim[i < 7 ? 0 : i - 7];
  RC_CHECK(compare(sim, shifted, 20).lag == 7);                                    // a trace delayed by 7 is found at 7

  // In a closed loop a late sensor is not a late copy of the same trace: the controller reacts to it, and the dynamics change.
  const Metrics closed = compare(run(0.1, 2.0, 0.0, 0, 1), run(0.1, 2.0, 0.0, 5, 1), 20);
  std::cout << "  the same delay inside a closed loop: error lined up " << closed.rmse << ", best shift " << closed.lag << ", error at that shift " << closed.rmse_at_lag << "\n\n";
  RC_CHECK(closed.rmse_at_lag > 0.5 * closed.rmse);
}

RC_TEST("the noise of the measurement sets a floor that no model can go below, and a perfect model scores right on it") {
  const double sigma = 0.01;
  const std::vector<double> clean = run(0.1, 2.0, 0.0, 0, 1);
  const std::vector<double> a = run(0.1, 2.0, sigma, 0, 11);
  const std::vector<double> b = run(0.1, 2.0, sigma, 0, 12);
  const double floor = noise_floor(a, b);
  const Metrics perfect = compare(clean, a, 0);
  std::cout << "  sensor noise " << sigma << ": two real runs give a noise floor of " << std::fixed << std::setprecision(4) << floor << "; a perfect noiseless model scores an error of " << perfect.rmse << " against a real run\n\n";
  RC_CHECK_NEAR(floor, sigma, 0.1 * sigma);
  RC_CHECK_NEAR(perfect.rmse, sigma, 0.1 * sigma);
  RC_CHECK_NEAR(perfect.rmse, floor, 0.15 * floor);
  RC_CHECK_NEAR(noise_floor(a, a), 0.0, 1e-12);
  RC_CHECK_NEAR(noise_floor(std::vector<double>(), a), 0.0, 1e-12);
}

RC_TEST("the error is smallest for the model that is right, and grows each way as the time constant is wrong, though the loop looks much the same") {
  const double sigma = 0.01;
  const std::vector<double> real = run(0.12, 2.0, sigma, 0, 21);
  const std::vector<double> real2 = run(0.12, 2.0, sigma, 0, 22);
  std::cout << "  the real motor has a time constant of 0.12 s, noise floor " << std::fixed << std::setprecision(4) << noise_floor(real, real2) << "\n  model's time constant   rmse    above the floor\n";
  double best = 1e9;
  double best_tau = 0.0;
  double at_truth = 0.0;
  double at_far = 0.0;
  const double floor = noise_floor(real, real2);
  for (double tau : {0.05, 0.08, 0.10, 0.12, 0.15, 0.20, 0.30}) {
    const Metrics m = compare(run(tau, 2.0, 0.0, 0, 1), real, 0);
    std::cout << "  " << std::setw(20) << std::setprecision(2) << tau << "   " << std::setw(6) << std::setprecision(4) << m.rmse << "   " << std::setw(8) << m.rmse - floor << "\n";
    if (m.rmse < best) {
      best = m.rmse;
      best_tau = tau;
    }
    if (tau == 0.12) at_truth = m.rmse;
    if (tau == 0.30) at_far = m.rmse;
  }
  std::cout << "\n";
  RC_CHECK_NEAR(best_tau, 0.12, 1e-9);
  RC_CHECK_NEAR(at_truth, floor, 0.2 * floor);
  RC_CHECK(at_far > 3.0 * at_truth);
}

RC_TEST("limits name what a simulation exceeded, and a simulation that fits the bench stays inside them") {
  const std::vector<double> real = run(0.12, 1.9, 0.005, 2, 31);
  const std::vector<double> good = run(0.12, 1.9, 0.0, 2, 1);
  const std::vector<double> poor = run(0.05, 2.0, 0.0, 0, 1);
  const Limits limits{0.02, 0.05, 1};
  const Metrics mg = compare(good, real, 20);
  const Metrics mp = compare(poor, real, 20);
  std::cout << "  limits: rmse 0.02, largest 0.05, lag 1\n  a model fitted to the bench: rmse " << std::fixed << std::setprecision(4) << mg.rmse << ", largest " << mg.max_abs << ", lag " << mg.lag
            << "; exceeded: " << joined(exceeded(mg, limits)) << "\n  a model with the wrong motor and no sensor delay: rmse " << mp.rmse << ", largest " << mp.max_abs << ", lag " << mp.lag
            << "; exceeded: " << joined(exceeded(mp, limits)) << "\n\n";
  RC_CHECK(exceeded(mg, limits).empty());
  const std::vector<std::string> bad = exceeded(mp, limits);
  RC_CHECK(!bad.empty());
  RC_CHECK(bad.size() >= 2);
  Metrics m;
  m.rmse = 1.0;
  m.max_abs = 1.0;
  m.lag = -5;
  RC_CHECK(joined(exceeded(m, limits)) == "rmse max lag");
  m.lag = -1;
  RC_CHECK(joined(exceeded(m, limits)) == "rmse max");
}
