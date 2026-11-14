#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

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
    {"z_arm", -0.1},
    {"r_in", 0.0},
    {"r_out", 0.7200000000000004},
    {"z_low", 0.19999999999999998},
    {"tol_cm", 36.00000000000002},
    {"tol_lo_cm", 31.500000000000018},
    {"r_in_low", 0.0},
    {"r_out_low", 0.6300000000000003},
    {"n_grid", 576.0},
    {"agree", 99.82638888888889},
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

MobileManipulator make() {
  MobileManipulator m;
  m.arm = elbow_arm();
  m.mount = trans(0.15, 0.0, 0.30);
  m.tool = trans(0.0, 0.0, 0.10);
  return m;
}

}  // namespace

RC_TEST("the ring the arm reaches has an inner and an outer radius, and a lower target gives a narrower one") {
  const MobileManipulator m = make();
  const double z = -0.10, z_low = -0.30;
  const Annulus a = reach_annulus(m, z, 0.01);
  const Annulus lo = reach_annulus(m, z_low, 0.01);
  rec("z_arm", z, 2);
  rec("r_in", a.r_in, 2);
  rec("r_out", a.r_out, 2);
  rec("z_low", z - z_low, 2);
  rec("tol_cm", 100.0 * park_tolerance(a), 1);
  rec("tol_lo_cm", 100.0 * park_tolerance(lo), 1);
  RC_CHECK(a.r_out > 0.4 && a.r_out < 0.9);
  RC_CHECK(a.r_in >= 0.0 && a.r_in < a.r_out);
  RC_CHECK(park_tolerance(a) > 0.1);
  rec("r_in_low", lo.r_in, 2);
  rec("r_out_low", lo.r_out, 2);
  RC_CHECK(lo.r_out > 0.0);
  RC_CHECK(park_tolerance(lo) < park_tolerance(a));
  RC_CHECK_NEAR(park_tolerance(a), 0.5 * (a.r_out - a.r_in), 1e-12);
}

RC_TEST("a point inside the ring is reached from the base and one outside is not, whichever way the base faces") {
  const MobileManipulator m = make();
  const Annulus a = reach_annulus(m, -0.10, 0.01);
  BasePose b;
  const double r_mid = 0.5 * (a.r_in + a.r_out);
  for (int k = 0; k < 8; ++k) {
    b.heading = k * 0.7853981633974483;
    const Pose axis = arm_base_in_world(m, b);
    const double inside[3] = {axis.p[0] + r_mid, axis.p[1], 0.20};
    const double outside[3] = {axis.p[0] + a.r_out + 0.15, axis.p[1], 0.20};
    RC_CHECK(arm_can_reach(m, b, inside));
    RC_CHECK(!arm_can_reach(m, b, outside));
  }
}

RC_TEST("the quick test agrees with the full inverse kinematics") {
  const MobileManipulator m = make();
  const Annulus a = reach_annulus(m, -0.10, 0.01);
  const double target[3] = {1.0, 0.5, 0.20};
  int agree = 0, n = 0;
  std::vector<BasePose> places;
  for (double x = -0.2; x <= 2.2; x += 0.1)
    for (double y = -0.7; y <= 1.7; y += 0.1) {
      BasePose b;
      b.x = x;
      b.y = y;
      b.heading = 0.3 * x - 0.2 * y;
      places.push_back(b);
    }
  for (std::size_t i = 0; i < places.size(); ++i) {
    ++n;
    if (arm_can_reach(m, places[i], target) == annulus_can_reach(m, a, places[i], target)) ++agree;
  }
  rec("n_grid", n, 0);
  rec("agree", 100.0 * agree / n, 0);
  RC_CHECK(agree >= 0.98 * n);
}
