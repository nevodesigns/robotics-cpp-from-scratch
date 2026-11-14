#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include <rc/core/random.hpp>
#include <rc/field/sensors.hpp>
#include <rc/vehicle/diff_slip.hpp>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"err_one", 0.3098373353995654},
    {"err_both", 5.773159728050814e-13},
    {"std1", 0.0034052999682444495},
    {"std4", 0.0019380163593174188},
    {"scale", 0.979431929480901},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;
const double kEff = 0.78;
const double kTrack = 0.160;
const double kTrue = kTrack / kEff;

// One full spin in place in a direction (+1 left, -1 right) on a skid steer of efficiency 0.78, measured by a gyro with the given bias and noise.
TurnRun spin(double direction, double bias, double sigma, unsigned int seed, double scrub = 0.0) {
  // The robot does not scrub the same way twice: the efficiency of a spin varies about its mean by a fraction (scrub).
  rc::core::Rng floor(seed * 7919u + 13u);
  const double eff = kEff * (1.0 + scrub * floor.gaussian());
  rc::vehicle::DiffSlip model(0.033, kTrack, 0.0, eff);
  rc::field::ImuSpec spec;
  spec.gyro_bias = bias;
  spec.gyro_sigma = sigma;
  spec.bias_walk = 0.0;
  rc::field::Imu imu(spec, seed);
  const double wheel_speed = 10.0 * direction;
  const double rate = eff * 0.033 * 2.0 * 10.0 / kTrack;  // the true turn rate, rad/s
  const double seconds = 2.0 * kPi / rate;
  const int n = static_cast<int>(seconds / 0.001 + 0.5);
  TurnRun run;
  double measured = 0.0;
  for (int i = 0; i < n; ++i) {
    model.step(-wheel_speed, wheel_speed, 0.001);
    run.wheel_diff += 0.033 * (2.0 * wheel_speed) * 0.001;
    const double w_true = direction * rate;
    measured += imu.measure(w_true, 0.0, 0.0, 0.001).gyro_z * 0.001;
  }
  run.dtheta = measured;
  return run;
}

}  // namespace

RC_TEST("with a perfect gyro one spin gives the effective track exactly") {
  const TurnRun r = spin(1.0, 0.0, 0.0, 1);
  const double b = calibrate_effective_track(std::vector<TurnRun>(1, r));
  RC_CHECK_NEAR(b, kTrue, 2e-4);
  RC_CHECK_NEAR(calibrate_effective_track(std::vector<TurnRun>(1, spin(-1.0, 0.0, 0.0, 1))), kTrue, 2e-4);
  RC_CHECK(calibrate_effective_track(std::vector<TurnRun>()) == 0.0);
  RC_CHECK(kTrue > kTrack);
}

RC_TEST("a gyro bias spoils a spin one way and cancels in a spin each way") {
  const double bias = 0.01;
  const double one = calibrate_effective_track(std::vector<TurnRun>(1, spin(1.0, bias, 0.0, 1)));
  std::vector<TurnRun> both;
  both.push_back(spin(1.0, bias, 0.0, 1));
  both.push_back(spin(-1.0, bias, 0.0, 1));
  const double two = calibrate_effective_track(both);
  const double err_one = std::fabs(one / kTrue - 1.0) * 100.0;
  const double err_both = std::fabs(two / kTrue - 1.0) * 100.0;
  rec("err_one", err_one, 2);
  rec("err_both", err_both, 3);
  RC_CHECK(err_one > 0.3);
  RC_CHECK(err_both < 0.01);
}

RC_TEST("more spins narrow the spread that the floor causes") {
  std::vector<double> one, four;
  for (unsigned int trial = 1; trial <= 30; ++trial) {
    std::vector<TurnRun> a;
    std::vector<TurnRun> b;
    for (int k = 0; k < 4; ++k) {
      const unsigned int seed = trial * 100 + static_cast<unsigned int>(k);
      b.push_back(spin(1.0, 0.01, 0.005, seed, 0.03));
      b.push_back(spin(-1.0, 0.01, 0.005, seed + 50, 0.03));
      if (k == 0) {
        a.push_back(b[0]);
        a.push_back(b[1]);
      }
    }
    one.push_back(calibrate_effective_track(a));
    four.push_back(calibrate_effective_track(b));
  }
  const Spread s1 = spread_of(one);
  const Spread s4 = spread_of(four);
  rec("std1", s1.stddev, 4);
  rec("std4", s4.stddev, 4);
  std::cout << "  true " << kTrue << ", one spin each way: " << s1.mean << " +- " << s1.stddev << ", four: " << s4.mean << " +- " << s4.stddev << "\n";
  RC_CHECK_NEAR(s1.mean, kTrue, 0.006);
  RC_CHECK_NEAR(s4.mean, kTrue, 0.004);
  RC_CHECK(s4.stddev < 0.75 * s1.stddev);
}

RC_TEST("the scale is the tape over the wheels, and the spread is the mean and the standard deviation") {
  const double scale = calibrate_scale(10.21, 10.00);
  rec("scale", scale, 4);
  RC_CHECK_NEAR(scale, 10.0 / 10.21, 1e-12);
  RC_CHECK(calibrate_scale(0.0, 5.0) == 0.0);
  const Spread s = spread_of(std::vector<double>({1.0, 2.0, 3.0, 4.0}));
  RC_CHECK_NEAR(s.mean, 2.5, 1e-12);
  RC_CHECK_NEAR(s.stddev, std::sqrt(1.25), 1e-12);
  RC_CHECK(spread_of(std::vector<double>()).mean == 0.0);
}
