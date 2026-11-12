#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>

#include "solution.hpp"

namespace {

struct Noise {
  std::uint64_t s;
  explicit Noise(std::uint64_t seed) : s(seed) {}
  double uniform() {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    return (static_cast<double>(s >> 11) + 0.5) / 9007199254740992.0;
  }
  double gauss(double sigma) { return sigma * std::sqrt(-2.0 * std::log(uniform())) * std::cos(6.283185307179586 * uniform()); }
};

const double kDeg = 3.14159265358979323846 / 180.0;
const double kNaN = std::numeric_limits<double>::quiet_NaN();

// A vehicle on level ground in a steady turn, flat (no lean), a sideways acceleration of lat, a gyro with a bias, an accelerometer with vibration. The true roll is zero throughout.
// Returns the largest absolute roll error, in degrees, over the last half of the run.
double run_flat(double tau, double gate, bool use_filter, double lat, double bias_deg_s, double seconds) {
  Noise n(7);
  TiltEstimator est(tau, gate);
  const double dt = 0.01;
  double worst = 0.0;
  const int steps = static_cast<int>(seconds / dt);
  for (int i = 0; i < steps; ++i) {
    const double ax = n.gauss(0.3);
    const double ay = lat + n.gauss(0.3);
    const double az = kGravity + n.gauss(0.3);
    const double gyro = bias_deg_s * kDeg;
    double angle;
    if (use_filter) angle = est.update(gyro, ax, ay, az, dt);
    else angle = accel_roll(ay, az);
    if (i >= steps / 2) worst = std::max(worst, std::fabs(angle) / kDeg);
  }
  return worst;
}

}  // namespace

RC_TEST("a vehicle tips when the sideways pull outweighs its weight, and a payload up high lowers both the angle and the pull it can take") {
  std::cout << "  track 0.5 m                 tips on a slope of   tips at a sideways acceleration of\n";
  const double heights[] = {0.3, 0.4, 0.5};
  double a30 = 0.0;
  double a50 = 0.0;
  for (double h : heights) {
    const double ang = static_stability_angle(0.5, h) / kDeg;
    const double acc = tip_accel(0.5, h);
    std::cout << "  centre of gravity " << std::fixed << std::setprecision(1) << h << " m   " << std::setw(8) << ang << " degrees   " << std::setprecision(2) << acc << " m/s^2\n";
    if (h == 0.3) a30 = acc;
    if (h == 0.5) a50 = acc;
  }
  RC_CHECK_NEAR(static_stability_angle(0.5, 0.3) / kDeg, 39.8, 0.05);
  RC_CHECK_NEAR(tip_accel(0.5, 0.3), 8.175, 1e-3);
  RC_CHECK_NEAR(static_stability_angle(0.5, 0.25), 45.0 * kDeg, 1e-9);        // the track equal to twice the height: 45 degrees
  RC_CHECK(a50 < a30);
  std::cout << "\n  a turn of radius 1 m, using half of what would tip it:\n";
  const double v30 = max_turn_speed(1.0, 0.5, 0.3, 0.5);
  const double v50 = max_turn_speed(1.0, 0.5, 0.5, 0.5);
  std::cout << "  centre of gravity 0.3 m: " << std::setprecision(2) << v30 << " m/s; with a payload that lifts it to 0.5 m: " << v50 << " m/s (" << std::setprecision(0) << 100.0 * v50 / v30 << " percent)\n";
  std::cout << "  a radius of 4 m, centre of gravity 0.3 m: " << std::setprecision(2) << max_turn_speed(0.25, 0.5, 0.3, 0.5) << " m/s\n\n";
  RC_CHECK(v50 < v30);
  RC_CHECK_NEAR(max_turn_speed(-1.0, 0.5, 0.3, 0.5), v30, 1e-12);
  RC_CHECK(std::isinf(max_turn_speed(0.0, 0.5, 0.3, 0.5)));
  RC_CHECK_NEAR(max_turn_speed(0.25, 0.5, 0.3, 0.5), 2.0 * v30, 1e-9);        // the speed goes as the square root of the radius
}

RC_TEST("standing still on a slope, the accelerometer gives the roll and the pitch") {
  const double angles[] = {5.0, 20.0, 35.0};
  for (double d : angles) {
    const double a = d * kDeg;
    RC_CHECK_NEAR(accel_roll(kGravity * std::sin(a), kGravity * std::cos(a)), a, 1e-12);
    RC_CHECK_NEAR(accel_pitch(-kGravity * std::sin(a), 0.0, kGravity * std::cos(a)), a, 1e-12);
  }
  RC_CHECK_NEAR(accel_roll(0.0, kGravity), 0.0, 1e-12);
  RC_CHECK_NEAR(accel_pitch(0.0, 0.0, kGravity), 0.0, 1e-12);
  RC_CHECK(accel_roll(-1.0, 9.0) < 0.0);
}

RC_TEST("on level ground in a turn the accelerometer alone reports a lean that is not there, and the filter's gate decides how far it is fooled") {
  const double lat = 4.0;             // a steady turn at 4 m/s^2 sideways
  const double accel_only = run_flat(0.0, 0.0, false, lat, 0.0, 10.0);
  std::cout << "  level ground, 4 m/s^2 sideways for 10 s (true roll 0), the worst error in the second half:\n  accelerometer alone                       " << std::fixed << std::setprecision(1) << accel_only << " degrees\n";
  const double g15 = run_flat(2.0, 0.15, true, lat, 0.0, 10.0);
  const double g03 = run_flat(2.0, 0.03, true, lat, 0.0, 10.0);
  std::cout << "  filter, tau 2 s, gate 15 percent of g     " << g15 << " degrees\n  filter, tau 2 s, gate 3 percent of g      " << g03 << " degrees\n\n";
  RC_CHECK(accel_only > 20.0);
  RC_CHECK(g15 > 15.0);               // a turn at 4 m/s^2 leaves |a| only 8 percent over g, inside a wide gate
  RC_CHECK(g03 < 8.0);                // vibration still lets the odd sample through a 3 percent gate
  RC_CHECK(g03 < g15 / 3.0);
}

RC_TEST("a gate tight enough to ignore a turn lets the gyro's bias build up, and a wider gate pays for it in the turn") {
  const double bias = 0.5;            // degrees per second of gyro bias
  const double gyro_only = 0.5 * 60.0;
  const double still_g15 = run_flat(2.0, 0.15, true, 0.0, bias, 60.0);
  const double still_g03 = run_flat(2.0, 0.03, true, 0.0, bias, 60.0);
  const double still_g01 = run_flat(2.0, 0.01, true, 0.0, bias, 60.0);
  std::cout << "  standing still on level ground for 60 s, a gyro bias of 0.5 deg/s, vibration of 0.3 m/s^2 on the accelerometer:\n";
  std::cout << "  gyro alone would reach                    " << std::setprecision(1) << gyro_only << " degrees\n";
  std::cout << "  filter, gate 15 percent                   " << still_g15 << " degrees\n  filter, gate 3 percent                    " << still_g03 << " degrees\n  filter, gate 1 percent                    " << still_g01 << " degrees\n\n";
  RC_CHECK(still_g15 < 3.0);
  RC_CHECK(still_g03 < still_g01);
  RC_CHECK(still_g01 > 2.0 * still_g15);
  RC_CHECK(still_g01 < gyro_only);
  const double turn_g01 = run_flat(2.0, 0.01, true, 4.0, 0.0, 10.0);
  std::cout << "  the same 1 percent gate in the 4 m/s^2 turn of the last test (no bias): " << turn_g01 << " degrees\n\n";
  RC_CHECK(turn_g01 < 5.0);
}

RC_TEST("the monitor warns early, stops only on a run of samples past the stop level, and stops at once on a reading that is not a number") {
  const double warn = 15.0 * kDeg;
  const double stop = 25.0 * kDeg;
  TiltMonitor m(warn, stop, 3);
  RC_CHECK(m.update(0.0) == TiltState::ok);
  RC_CHECK(m.update(10.0 * kDeg) == TiltState::ok);
  RC_CHECK(m.update(16.0 * kDeg) == TiltState::warn);
  RC_CHECK(m.update(-16.0 * kDeg) == TiltState::warn);             // either side
  RC_CHECK(m.update(30.0 * kDeg) == TiltState::warn);              // one sample past the stop level: a bump
  RC_CHECK(m.update(10.0 * kDeg) == TiltState::ok);
  RC_CHECK(m.update(30.0 * kDeg) == TiltState::warn);
  RC_CHECK(m.update(30.0 * kDeg) == TiltState::warn);
  RC_CHECK(m.update(-30.0 * kDeg) == TiltState::stop);             // the third in a row
  RC_CHECK(m.update(kNaN) == TiltState::stop);
  TiltMonitor fresh(warn, stop, 3);
  RC_CHECK(fresh.update(kNaN) == TiltState::stop);
  std::cout << "  one sample at 30 degrees was a warning, three in a row a stop, a NaN a stop at once\n\n";
}
