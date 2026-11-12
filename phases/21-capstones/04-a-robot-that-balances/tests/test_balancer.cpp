#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

const double kDeg = 3.14159265358979323846 / 180.0;

BalanceResult run(BalanceConfig c, double seconds = 10.0) { return run_balance(c, seconds); }

// The steepest initial lean, in degrees, that is recovered (it does not fall), found by halving.
double boundary(double a_max, bool supervisor = false) {
  double lo = 2.0;
  double hi = 40.0;
  for (int i = 0; i < 20; ++i) {
    const double mid = 0.5 * (lo + hi);
    BalanceConfig c;
    c.a_max = a_max;
    c.initial_tilt = mid * kDeg;
    c.use_supervisor = supervisor;
    if (run(c).fell) hi = mid;
    else lo = mid;
  }
  return lo;
}

}  // namespace

RC_TEST("a broom on a hand falls the way it leans, is held by moving the hand under it, and the wheel acceleration that holds a lean is g times its tangent") {
  RC_CHECK_NEAR(pendulum_accel(0.0, 0.0, 9.81, 0.15), 0.0, 1e-12);
  RC_CHECK_NEAR(pendulum_accel(0.1, 0.0, 9.81, 0.15), 9.81 * std::sin(0.1) / 0.15, 1e-12);
  RC_CHECK(pendulum_accel(0.1, 0.0, 9.81, 0.15) > 0.0);                          // leaning forward, it falls forward faster
  RC_CHECK(pendulum_accel(0.1, 3.0, 9.81, 0.15) < pendulum_accel(0.1, 0.0, 9.81, 0.15));       // moving the axle forward pushes it back
  RC_CHECK_NEAR(pendulum_accel(0.3, 9.81 * std::tan(0.3), 9.81, 0.15), 0.0, 1e-9);            // g tan(theta) holds it
  RC_CHECK_NEAR(pendulum_accel(-0.2, 0.0, 9.81, 0.3), -9.81 * std::sin(0.2) / 0.3, 1e-12);
  RC_CHECK_NEAR(max_recoverable_tilt(4.0, 9.81), std::atan(4.0 / 9.81), 1e-12);
  RC_CHECK(max_recoverable_tilt(0.0, 9.81) == 0.0 && max_recoverable_tilt(-1.0, 9.81) == 0.0);
  std::cout << "  a motor limit of 4 m/s^2 can hold a lean of up to atan(4 / 9.81) = " << std::fixed << std::setprecision(1) << max_recoverable_tilt(4.0, 9.81) / kDeg << " degrees, at the very most\n\n";
}

RC_TEST("the command pushes the wheels the way the body leans, harder than gravity, and aims to lean back when it is going forward") {
  RC_CHECK_NEAR(balance_command(25.0, 3.0, 0.05, 0.05, 0.0, 0.0), 1.25, 1e-12);
  RC_CHECK_NEAR(balance_command(25.0, 3.0, 0.05, -0.05, 0.0, 0.0), -1.25, 1e-12);
  RC_CHECK_NEAR(balance_command(25.0, 3.0, 0.05, 0.0, 1.0, 0.0), 3.0, 1e-12);
  RC_CHECK_NEAR(balance_command(25.0, 3.0, 0.05, 0.0, 0.0, 1.0), 25.0 * 0.05, 1e-12);        // going forward at 1 m/s: the aim is 0.05 rad back, and to tip the body back the axle is pushed forward first
  RC_CHECK_NEAR(balance_command(25.0, 3.0, 0.05, 0.0, 0.0, 10.0), 25.0 * 0.1, 1e-12);         // the aim stops at 0.1 radians
  RC_CHECK_NEAR(balance_command(25.0, 3.0, 0.05, 0.0, 0.0, -10.0), -25.0 * 0.1, 1e-12);
  RC_CHECK(25.0 > 9.81);
}

RC_TEST("the steepest lean it is caught from is well under the sum, and the sum is only a bound") {
  std::cout << "  the steepest initial lean that is recovered, by the motor's limit on acceleration:\n  a_max       measured     atan(a_max / g)\n";
  const double limits[] = {2.0, 4.0, 8.0};
  double measured[3];
  for (int i = 0; i < 3; ++i) {
    measured[i] = boundary(limits[i]);
    std::cout << "  " << std::fixed << std::setprecision(1) << std::setw(4) << limits[i] << " m/s^2  " << std::setw(6) << measured[i] << " deg    " << std::setw(6) << max_recoverable_tilt(limits[i], 9.81) / kDeg << " deg\n";
    RC_CHECK(measured[i] < max_recoverable_tilt(limits[i], 9.81) / kDeg);
  }
  std::cout << "\n";
  RC_CHECK(measured[0] < measured[1] && measured[1] < measured[2]);
  RC_CHECK(measured[1] > 15.5 && measured[1] < 17.5);
  BalanceConfig easy;
  easy.initial_tilt = 15.0 * kDeg;
  easy.use_supervisor = false;
  RC_CHECK(!run(easy).fell);
  BalanceConfig hard = easy;
  hard.initial_tilt = 18.0 * kDeg;
  RC_CHECK(run(hard).fell);
  RC_CHECK(measured[2] < 0.7 * max_recoverable_tilt(8.0, 9.81) / kDeg);       // at a high limit the speed limit of the axle is what bites
}

RC_TEST("a delay between the filter and the wheels is harmless up to 50 ms and fatal from 55, because the body falls with a time constant of 0.12 s") {
  std::cout << "  a delay between the command and the wheels:\n  delay    result\n";
  const int delays[] = {0, 4, 8, 10, 11, 12, 14};
  bool fell[7];
  for (int i = 0; i < 7; ++i) {
    BalanceConfig c;
    c.delay_frames = delays[i];
    c.use_supervisor = false;
    const BalanceResult r = run(c);
    fell[i] = r.fell;
    std::cout << "  " << std::setw(3) << delays[i] * 5 << " ms   " << (r.fell ? "FELL at " + std::to_string(r.fell_at).substr(0, 4) + " s" : "stands, rms tilt " + std::to_string(r.rms_tilt).substr(0, 6)) << "\n";
  }
  std::cout << "  (the body's time constant is sqrt(L / g) = " << std::setprecision(3) << std::sqrt(0.15 / 9.81) << " s)\n\n";
  RC_CHECK(!fell[0] && !fell[1] && !fell[2] && !fell[3]);
  RC_CHECK(fell[4] && fell[5] && fell[6]);
}

RC_TEST("a filter that is too quick passes the accelerometer's noise to the wheels, and one that is too slow lets the gyro's bias build up until it falls") {
  std::cout << "  gyro bias 0.05 rad/s, accelerometer noise 0.05 rad, 20 s:\n  tau (s)   result\n";
  const double taus[] = {0.05, 0.2, 0.5, 2.0, 5.0, 1000.0};
  BalanceResult r[6];
  for (int i = 0; i < 6; ++i) {
    BalanceConfig c;
    c.filter_tau = taus[i];
    c.gyro_bias = 0.05;
    c.accel_noise = 0.05;
    c.use_supervisor = false;
    r[i] = run(c, 20.0);
    std::cout << "  " << std::fixed << std::setprecision(2) << std::setw(7) << taus[i] << "   " << (r[i].fell ? "FELL at " + std::to_string(r[i].fell_at).substr(0, 4) + " s" : "stands, drifted " + std::to_string(r[i].distance).substr(0, 5) + " m") << "\n";
  }
  std::cout << "\n";
  RC_CHECK(r[0].fell && r[0].fell_at < 2.0);
  RC_CHECK(!r[1].fell && !r[2].fell);
  RC_CHECK(std::fabs(r[1].distance) > 3.0 && std::fabs(r[2].distance) > 3.0);        // upright, and slowly going away: a bias makes it lean to one side
  RC_CHECK(r[3].fell && r[3].fell_at > 5.0);
  RC_CHECK(r[4].fell && r[5].fell);
  RC_CHECK(r[5].fell_at < r[3].fell_at);
}

RC_TEST("a lean past 25 degrees stops the motors, and a robot that goes on trying runs away at the axle's top speed while it falls") {
  BalanceConfig with;
  with.initial_tilt = 25.0 * kDeg;
  BalanceConfig without = with;
  without.use_supervisor = false;
  const BalanceResult a = run(with);
  const BalanceResult b = run(without);
  std::cout << "  started 25 degrees over, to see what happens when it cannot be caught:\n";
  std::cout << "  supervisor: stopped at " << std::fixed << std::setprecision(3) << a.stopped_at << " s (cause " << rc::safety::causes_text(a.causes) << "), the axle went " << std::setprecision(2) << a.distance << " m, up to " << a.max_speed << " m/s, " << a.distance_after_fall << " m while it fell\n";
  std::cout << "  none:       the axle went " << b.distance << " m, up to " << b.max_speed << " m/s, " << b.distance_after_fall << " m while it fell\n";
  RC_CHECK(a.stopped && a.causes == rc::safety::cause_tilt);
  RC_CHECK(a.stopped_at < a.fell_at);
  RC_CHECK(b.max_speed > 1.4);
  RC_CHECK(a.max_speed < 0.8 * b.max_speed);
  RC_CHECK(a.distance_after_fall < 0.3 * b.distance_after_fall);
  BalanceConfig dead;
  dead.imu_dead_at = 2.0;
  const BalanceResult d = run(dead);
  BalanceConfig dead_without = dead;
  dead_without.use_supervisor = false;
  const BalanceResult e = run(dead_without);
  std::cout << "  the IMU dead at 2 s: with the supervisor a stop at " << std::setprecision(3) << d.stopped_at << " s, cause " << rc::safety::causes_text(d.causes) << "; without, no stop and no cause, and it falls at " << std::setprecision(2) << e.fell_at << " s all the same\n\n";
  RC_CHECK(d.stopped && d.causes == rc::safety::cause_sensors);
  RC_CHECK(d.stopped_at > 2.0 && d.stopped_at < 2.05);
  RC_CHECK(!e.stopped && e.fell);
}
