#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

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
    {"closure_ok", 0.023429451336518376},
    {"turn_err", 11.134020618568865},
    {"scale_err", 0.19999999999997442},
    {"closure_ideal", 0.023429451336518376},
    {"truth_gap", 4.639696570517785},
    {"cal_err", 0.0},
    {"steer_turn", 60.575502817304404},
    {"steer_lat", 9.623602134575433},
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

const double kR = 0.033;
const double kB = 0.160;

// A square of 10 m sides driven by a model with losses, the odometry fed with the distance the wheels rolled.
struct Run {
  Pose2 truth;
  Pose2 estimate;
};

Run drive_square(double slip, double eff, double odo_track, double odo_scale) {
  DiffSlip model(kR, kB, slip, eff);
  WheelOdometry odo(odo_track, odo_scale);
  const double dt = 0.001;
  auto leg = [&](double wl, double wr, double seconds) {
    const int n = static_cast<int>(seconds / dt + 0.5);
    for (int i = 0; i < n; ++i) {
      model.step(wl, wr, dt);
      odo.update(kR * wl * dt, kR * wr * dt);
    }
  };
  for (int side = 0; side < 4; ++side) {
    leg(20.0, 20.0, 10.0 / (kR * 20.0));
    leg(-10.0, 10.0, (kPi / 2.0) / (kR * 20.0 / kB));
  }
  Run r;
  r.truth = model.pose();
  r.estimate = odo.pose();
  return r;
}

double home(const Pose2& p) { return std::hypot(p.x, p.y); }

}  // namespace

RC_TEST("with the parameters of the model the odometry is the model") {
  const Run r = drive_square(0.0, 1.0, kB, 1.0);
  rec("closure_ok", home(r.estimate), 3);
  RC_CHECK_NEAR(r.estimate.x, r.truth.x, 1e-9);
  RC_CHECK_NEAR(r.estimate.y, r.truth.y, 1e-9);
  RC_CHECK_NEAR(r.estimate.theta, r.truth.theta, 1e-9);
  RC_CHECK(home(r.estimate) < 0.05);
}

RC_TEST("a track width 3 percent too small makes a full turn read 3 percent too much, and a wheel scale out reads the distance out") {
  // One full turn in place: the wheels travel the same either way, and the heading is the travel over the track.
  WheelOdometry right(kB, 1.0);
  WheelOdometry small(0.97 * kB, 1.0);
  const double travel = kPi * kB;  // each wheel rolls half a circle of the track
  for (int i = 0; i < 1000; ++i) {
    right.update(-travel / 1000.0, travel / 1000.0);
    small.update(-travel / 1000.0, travel / 1000.0);
  }
  RC_CHECK_NEAR(right.pose().theta, 2.0 * kPi, 1e-9);
  const double err = (small.pose().theta - right.pose().theta) * 180.0 / kPi;
  rec("turn_err", err, 1);
  RC_CHECK_NEAR(err, 360.0 * (1.0 / 0.97 - 1.0), 1e-6);

  WheelOdometry out(kB, 1.02);
  for (int i = 0; i < 1000; ++i) out.update(0.01, 0.01);
  rec("scale_err", out.pose().x - 10.0, 2);
  RC_CHECK_NEAR(out.pose().x, 10.2, 1e-9);
}

RC_TEST("against slip and a poor turn the ideal parameters do not close the square and the calibrated ones do") {
  const Run ideal = drive_square(0.02, 0.9, kB, 1.0);
  const Run calibrated = drive_square(0.02, 0.9, kB / 0.9, 0.98);
  rec("closure_ideal", home(ideal.estimate), 2);
  rec("truth_gap", home(ideal.truth), 1);
  rec("cal_err", std::hypot(calibrated.estimate.x - calibrated.truth.x, calibrated.estimate.y - calibrated.truth.y), 3);
  std::cout << "  the truth ended " << home(ideal.truth) << " m from home; the ideal odometry believed " << home(ideal.estimate) << " m and the calibrated one " << home(calibrated.estimate) << " m\n";
  RC_CHECK(home(ideal.truth) > 3.0);
  // The ideal odometry believes it came home, and it did not: the error is the truth's distance from home.
  RC_CHECK(home(ideal.estimate) < 0.05);
  RC_CHECK(std::hypot(ideal.estimate.x - ideal.truth.x, ideal.estimate.y - ideal.truth.y) > 3.0);
  RC_CHECK_NEAR(calibrated.estimate.x, calibrated.truth.x, 0.05);
  RC_CHECK_NEAR(calibrated.estimate.y, calibrated.truth.y, 0.05);
}

RC_TEST("a car with a steering sensor one degree off zero believes it is turning when it is not") {
  const double offset = 1.0 * kPi / 180.0;
  AckermannOdometry wrong(0.3302, 0.0);
  AckermannOdometry right(0.3302, offset);
  for (int i = 0; i < 2000; ++i) {
    // The wheels are straight, and the sensor reads the offset.
    wrong.update(0.01, offset);
    right.update(0.01, offset);
  }
  rec("steer_turn", wrong.pose().theta * 180.0 / kPi, 0);
  rec("steer_lat", wrong.pose().y, 1);
  RC_CHECK_NEAR(right.pose().theta, 0.0, 1e-12);
  RC_CHECK_NEAR(right.pose().x, 20.0, 1e-9);
  RC_CHECK_NEAR(wrong.pose().theta, 20.0 * std::tan(offset) / 0.3302, 1e-9);
  RC_CHECK(wrong.pose().theta * 180.0 / kPi > 55.0);
  RC_CHECK(wrong.pose().y > 8.0);
}
