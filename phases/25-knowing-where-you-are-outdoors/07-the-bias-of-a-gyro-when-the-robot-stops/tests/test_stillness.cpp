#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include <rc/core/random.hpp>
#include <rc/field/sensors.hpp>

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
    {"carried_wheels_only", 176.0},
    {"carried_both", 0.0},
    {"err_1s", 0.0006297819663715962},
    {"err_10s", 0.00020473933485481104},
    {"no_zupt", 9.940498667801283},
    {"zupt_60", 4.608741948591702},
    {"zupt_10", 0.7541477782987769},
    {"relearn", 18.540000000000006},
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

// 300 s of a robot that turns at 0.2 rad/s for 20 s of every 30 and is still for `stop` seconds every `every` seconds (none if every is 0). The gyro's bias walks. Returns the heading error at the end, in degrees.
double heading_error(double every, double stop, unsigned int seed, double step_at = -1.0, double* relearn = nullptr) {
  rc::field::ImuSpec spec;
  spec.gyro_bias = 0.01;
  spec.gyro_sigma = 0.005;
  spec.bias_walk = 3.0e-4;
  rc::field::Imu imu(spec, seed);
  const double dt = 0.02;
  StillnessDetector det(25, 0.002, 0.2);
  StillBiasFilter f(0.005, 3.0e-4, 0.0, 0.05);
  double th_true = 0.0, th_est = 0.0;
  // The bias is read once at the start, from the first 5 s still (every case has them).
  for (int k = 0; k < 6000 + 250; ++k) {
    const double t = (k - 250) * dt;
    const bool start_still = k < 250;
    bool still = start_still;
    if (!start_still && every > 0.0) {
      const double phase = std::fmod(t, every);
      still = phase > every - stop;
    }
    const double w_true = still ? 0.0 : 0.2 * std::sin(2.0 * kPi * t / 30.0);
    double gyro = imu.measure(w_true, 0.0, 0.0, dt).gyro_z;
    if (step_at >= 0.0 && t >= step_at) gyro += 0.01;
    const double accel = 9.81;
    const double wheel = still ? 0.0 : 0.5;
    const bool detected = det.update(wheel, wheel, accel);
    const bool use = start_still ? (k > 30) : (every > 0.0 ? detected : false);
    f.update(gyro, use, dt);
    if (!start_still) {
      th_true += w_true * dt;
      th_est += (gyro - f.bias()) * dt;
    }
    if (relearn != nullptr && step_at >= 0.0 && *relearn < 0.0 && t > step_at && use && std::fabs(f.bias() - (0.01 + 0.01)) < 0.003) *relearn = t - step_at;
  }
  return std::fabs(th_est - th_true) * 180.0 / kPi;
}

}  // namespace

RC_TEST("a robot is still when its wheels and its accelerometer are both quiet for a window") {
  StillnessDetector d(25, 0.002, 0.2);
  int still_after = -1;
  for (int k = 0; k < 100; ++k) {
    const bool s = d.update(0.0, 0.0, 9.81 + (k % 2 ? 0.01 : -0.01));
    if (s && still_after < 0) still_after = k + 1;
  }
  RC_CHECK(still_after == 25);
  RC_CHECK(!d.update(0.5, 0.5, 9.81));   // the wheels turn
  RC_CHECK(!d.update(0.0, 0.0, 9.81));   // and it takes a whole window to be sure again
  // Moving in a straight line keeps it from being still.
  StillnessDetector m(25, 0.002, 0.2);
  for (int k = 0; k < 100; ++k) RC_CHECK(!m.update(1.0, 1.0, 9.81));
}

RC_TEST("wheels that are stopped are not enough: a robot carried or dragged is not still") {
  StillnessDetector wheels_only(25, 0.002, 1e9);
  StillnessDetector with_accel(25, 0.002, 0.2);
  rc::core::Rng rng(3);
  int wheels_says = 0, both_says = 0;
  for (int k = 0; k < 200; ++k) {
    const double accel = 9.81 + 1.5 * rng.gaussian();  // carried: it swings
    if (wheels_only.update(0.0, 0.0, accel)) ++wheels_says;
    if (with_accel.update(0.0, 0.0, accel)) ++both_says;
  }
  rec("carried_wheels_only", wheels_says, 0);
  rec("carried_both", both_says, 0);
  RC_CHECK(wheels_says > 150);
  RC_CHECK(both_says < 5);
}

RC_TEST("the bias read from a stop is good to the noise over the square root of the samples") {
  double sq1 = 0.0, sq10 = 0.0;
  const int trials = 40;
  for (int tr = 0; tr < trials; ++tr) {
    rc::field::ImuSpec spec;
    spec.gyro_bias = 0.01;
    spec.gyro_sigma = 0.005;
    spec.bias_walk = 0.0;
    rc::field::Imu imu(spec, 100 + static_cast<unsigned int>(tr));
    StillBiasFilter f1(0.005, 0.0, 0.0, 0.05), f10(0.005, 0.0, 0.0, 0.05);
    for (int k = 0; k < 500; ++k) {
      const double g = imu.measure(0.0, 0.0, 0.0, 0.02).gyro_z;
      if (k < 50) f1.update(g, true, 0.02);
      f10.update(g, true, 0.02);
    }
    sq1 += (f1.bias() - 0.01) * (f1.bias() - 0.01);
    sq10 += (f10.bias() - 0.01) * (f10.bias() - 0.01);
  }
  const double e1 = std::sqrt(sq1 / trials), e10 = std::sqrt(sq10 / trials);
  rec("err_1s", e1, 5);
  rec("err_10s", e10, 5);
  RC_CHECK_NEAR(e1, 0.005 / std::sqrt(50.0), 0.0004);
  RC_CHECK(e10 < 0.5 * e1);
  RC_CHECK_NEAR(heading_error_growth(0.001, 300.0), 0.3, 1e-12);
}

RC_TEST("stopping for a couple of seconds now and then keeps the heading where reading the bias once does not") {
  double none = 0.0, every60 = 0.0, every10 = 0.0;
  for (unsigned int seed = 1; seed <= 5; ++seed) {
    none += heading_error(0.0, 0.0, seed);
    every60 += heading_error(60.0, 2.0, seed);
    every10 += heading_error(10.0, 2.0, seed);
  }
  rec("no_zupt", none / 5.0, 1);
  rec("zupt_60", every60 / 5.0, 1);
  rec("zupt_10", every10 / 5.0, 1);
  std::cout << "  heading error after 300 s, mean of five: " << none / 5.0 << " degrees reading the bias once, " << every60 / 5.0 << " stopping 2 s every 60 s, " << every10 / 5.0 << " every 10 s\n";
  RC_CHECK(every60 < none);
  RC_CHECK(every10 < every60);
}

RC_TEST("a step in the bias, as when the gyro warms, is found at the next stop") {
  double relearn = -1.0;
  const double err = heading_error(30.0, 2.0, 7, 100.0, &relearn);
  rec("relearn", relearn, 0);
  std::cout << "  the bias stepped by 0.01 rad/s at 100 s and was found again " << relearn << " s later; heading error " << err << " degrees\n";
  RC_CHECK(relearn > 0.0);
  RC_CHECK(relearn < 35.0);
}
