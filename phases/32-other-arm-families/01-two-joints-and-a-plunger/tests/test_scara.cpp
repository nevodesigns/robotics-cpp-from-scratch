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
    {"l1", 0.225},
    {"l2", 0.225},
    {"ok_n", 159.0},
    {"grid_n", 272.0},
    {"err_fk", 1.6243534171511178e-16},
    {"far", 0.5},
    {"far_ok", 0.0},
    {"det_mid", 0.050625},
    {"q2_mid", 1.5707963267948966},
    {"det_str", 0.0},
    {"mass", 2.0},
    {"g_scara", 0.0},
    {"reach_m", 0.4},
    {"g_six", 7.848000000000001},
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


RC_TEST("the inverse kinematics puts the tool where it was asked, for either elbow") {
  Scara s;
  int ok = 0, n = 0;
  double worst = 0.0;
  for (double x = -0.4; x <= 0.4 + 1e-9; x += 0.05)
    for (double y = 0.05; y <= 0.4 + 1e-9; y += 0.05) {
      ++n;
      for (int e = -1; e <= 1; e += 2) {
        double q1 = 0.0, q2 = 0.0;
        if (scara_ik(s, x, y, e, q1, q2)) {
          ++ok;
          const ScaraTool t = scara_fk(s, q1, q2, 0.0, 0.0);
          worst = std::fmax(worst, std::hypot(t.x - x, t.y - y));
        }
      }
    }
  rec("l1", s.l1, 3);
  rec("l2", s.l2, 3);
  rec("ok_n", ok, 0);
  rec("grid_n", 2 * n, 0);
  rec("err_fk", worst, 9);
  RC_CHECK(ok > n);
  RC_CHECK(worst < 1e-9);
  double a1 = 0.0, a2 = 0.0, b1 = 0.0, b2 = 0.0;
  RC_CHECK(scara_ik(s, 0.3, 0.2, 1, a1, a2) && scara_ik(s, 0.3, 0.2, -1, b1, b2));
  RC_CHECK_NEAR(a2, -b2, 1e-12);
  RC_CHECK(std::fabs(a1 - b1) > 0.1);
}

RC_TEST("a point out of reach, or one that the joint limits forbid, is refused") {
  Scara s;
  double q1 = 0.0, q2 = 0.0;
  rec("far", 0.5, 1);
  rec("far_ok", scara_ik(s, 0.5, 0.0, 1, q1, q2) ? 1 : 0, 0);
  RC_CHECK(!scara_ik(s, 0.5, 0.0, 1, q1, q2));
  RC_CHECK(!scara_ik(s, 0.0, 0.0, 1, q1, q2) || std::fabs(q2) > 3.0);  // folded flat on itself: q2 is pi, outside the limit
  Scara tight = s;
  tight.q2_hi = 0.2;
  tight.q2_lo = -0.2;
  RC_CHECK(!scara_ik(tight, 0.0, 0.3, 1, q1, q2));
}

RC_TEST("the determinant of the planar Jacobian is zero straight and folded, and the first two joints carry no weight") {
  Scara s;
  rec("det_mid", scara_det(s, 1.5707963267948966), 4);
  rec("q2_mid", 1.5707963267948966, 2);
  rec("det_str", scara_det(s, 0.0), 4);
  RC_CHECK_NEAR(scara_det(s, 0.0), 0.0, 1e-12);
  RC_CHECK_NEAR(scara_det(s, 3.141592653589793), 0.0, 1e-12);
  RC_CHECK_NEAR(scara_det(s, 1.5707963267948966), s.l1 * s.l2, 1e-12);
  const double m = 2.0, reach = 0.4;
  rec("mass", m, 0);
  rec("g_scara", gravity_torque(m), 1);
  rec("reach_m", reach, 1);
  rec("g_six", m * 9.81 * reach, 2);
  RC_CHECK_NEAR(gravity_torque(m), 0.0, 1e-12);
}
