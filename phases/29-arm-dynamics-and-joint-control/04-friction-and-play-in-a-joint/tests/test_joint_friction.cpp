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
    {"f_peak", 0.055938965846946784},
    {"peak_v", 0.017000000000000008},
    {"f_dip", 0.0502801400167021},
    {"dip_v", 0.052},
    {"f_fast", 0.055},
    {"err_plain", 1.3344237737174735e-12},
    {"err_ff", 1.3196612590084469e-14},
    {"play_true", 0.01},
    {"play_meas", 0.010162009124059498},
    {"pp_load", 0.0012716254598015286},
    {"off_load", 0.0006358993184519823},
    {"pp_motor", 0.0},
    {"off_motor", 0.0034073173674664115},
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


RC_TEST("friction rises to a peak at a low speed, dips as the joint slides, and then grows with the speed") {
  const FrictionModel f;
  double peak = 0.0, peak_v = 0.0;
  for (double v = 0.001; v < 0.1; v += 0.001) {
    const double t = friction_torque(f, v);
    if (t > peak) {
      peak = t;
      peak_v = v;
    }
  }
  double dip = 1e9, dip_v = 0.0;
  for (double v = peak_v + 0.005; v < 2.0; v += 0.005) {
    const double t = friction_torque(f, v);
    if (t < dip) {
      dip = t;
      dip_v = v;
    }
  }
  const double fast = friction_torque(f, 1.0);
  rec("f_peak", peak, 4);
  rec("peak_v", peak_v, 3);
  rec("f_dip", dip, 4);
  rec("dip_v", dip_v, 2);
  rec("f_fast", fast, 4);
  RC_CHECK(friction_torque(f, 0.0) == 0.0);
  RC_CHECK_NEAR(friction_torque(f, -0.3), -friction_torque(f, 0.3), 1e-12);
  RC_CHECK(peak > dip + 0.003);
  RC_CHECK(dip_v > peak_v);
  RC_CHECK(friction_torque(f, 2.0) > dip);
  RC_CHECK(peak_v < 0.06);
}

RC_TEST("feeding the friction forward makes a slow ramp followed much more closely") {
  TwoMass s;
  s.half_gap = 0.0;
  JointController plain, ff;
  ff.friction_ff = true;
  const RampResult a = track_ramp(s, plain, 0.03, 8.0);
  const RampResult b = track_ramp(s, ff, 0.03, 8.0);
  rec("err_plain", a.mean_error, 5);
  rec("err_ff", b.mean_error, 5);
  RC_CHECK(a.mean_error > 2.0 * b.mean_error);
  RC_CHECK(b.mean_error < 0.003);
}

RC_TEST("the gap of a gear is found from how far the motor goes before the load moves") {
  TwoMass s;
  const double meas = measure_play(s);
  rec("play_true", 2.0 * s.half_gap, 4);
  rec("play_meas", meas, 4);
  RC_CHECK_NEAR(meas, 2.0 * s.half_gap, 0.0015);
  s.half_gap = 0.0;
  RC_CHECK(measure_play(s) < 0.001);
}

RC_TEST("with play in the gear the loop on the load's encoder hunts, and the loop on the motor's is steady but leaves the load off") {
  TwoMass s;
  JointController on_load, on_motor;
  on_load.ki_pos = on_motor.ki_pos = 8.0;
  on_motor.feedback_load = false;
  const HoldResult a = hold(s, on_load, 0.5, 0.02, 12.0);
  const HoldResult b = hold(s, on_motor, 0.5, 0.02, 12.0);
  rec("pp_load", a.load_pp, 4);
  rec("off_load", a.final_error, 4);
  rec("pp_motor", b.load_pp, 6);
  rec("off_motor", b.final_error, 4);
  RC_CHECK(a.load_pp > 10.0 * b.load_pp + 1e-4);
  RC_CHECK(b.load_pp < 1e-3);
}
