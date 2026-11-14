#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

#include "rc/arm/ik_closed.hpp"
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
    {"load", 10.0},
    {"range0", 15508.971201317523},
    {"range1", 13838.048703341889},
    {"reach_m", 0.65},
    {"arm_kg", 5.0},
    {"tip_kg", 4.282120282789645},
    {"trac_kg", 130.7},
    {"v_stop", 1.5},
    {"d_stop", 0.2},
    {"a_req", 5.625},
    {"a_want", 0.5},
    {"best_kg", 4.282120282789645},
    {"binding", 1.0},
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
  for (std::size_t i = 0; i < m.arm.rows.size(); ++i) {
    m.arm.rows[i].lo = -3.0;
    m.arm.rows[i].hi = 3.0;
  }
  m.mount = trans(0.0, 0.0, 0.10);
  m.tool = trans(0.0, 0.0, 0.10);
  return m;
}

std::vector<double> pose_q(const MobileManipulator& m, double x, double y, double z) {
  Pose p;
  p.r[1][1] = -1.0;
  p.r[2][2] = -1.0;
  p.p[0] = x;
  p.p[1] = y;
  p.p[2] = z;
  const std::vector<std::vector<double> > sols = ik_spherical(m.arm, p, Pose(), m.tool);
  if (sols.empty()) return std::vector<double>();
  std::vector<double> best = sols[0];
  double bn = 1e9;
  for (std::size_t i = 0; i < sols.size(); ++i) {
    double n = 0.0;
    for (std::size_t k = 0; k < 6; ++k) n += sols[i][k] * sols[i][k];
    if (n < bn) {
      bn = n;
      best = sols[i];
    }
  }
  return best;
}

const double kLinks[6] = {3.0, 2.5, 2.0, 0.8, 0.6, 0.4};

MassModel masses() {
  MassModel mm;
  mm.link_mass.assign(kLinks, kLinks + 6);
  return mm;
}

ArmModel arm_model(const MobileManipulator& m) {
  ArmModel am;
  am.table = m.arm;
  am.links = rod_links(m.arm, std::vector<double>(kLinks, kLinks + 6), 0.04);
  return am;
}

}  // namespace

RC_TEST("the base's acceleration is the force over the mass until the floor's grip gives out") {
  BaseDrive d;
  RC_CHECK_NEAR(base_accel_limit(d, 40.0), 80.0 / 40.0, 1e-12);
  RC_CHECK_NEAR(base_accel_limit(d, 4.0), 0.6 * 9.81, 1e-12);
  RC_CHECK(base_accel_limit(d, 60.0) < base_accel_limit(d, 30.0));
}

RC_TEST("a payload shortens the range on a battery, through the rolling resistance") {
  BaseDrive d;
  const double m0 = 29.3, load = 10.0, v = 1.0;
  rec("load", load, 0);
  rec("range0", range_m(d, m0, v), 0);
  rec("range1", range_m(d, m0 + load, v), 0);
  RC_CHECK(range_m(d, m0 + load, v) < range_m(d, m0, v));
  RC_CHECK(range_m(d, m0, 2.0) > 0.0);
  // with no rolling resistance only the electronics' draw is left and the mass does not matter
  BaseDrive ideal = d;
  ideal.c_rr = 0.0;
  RC_CHECK_NEAR(range_m(ideal, 10.0, v), range_m(ideal, 100.0, v), 1e-9);
}

RC_TEST("the payload is the least of three limits, and the binding one is named") {
  const MobileManipulator m = make();
  BasePose b;
  const std::vector<double> q = pose_q(m, 0.65, 0.0, 0.30);
  RC_CHECK(!q.empty());
  const std::vector<double> limits = {60.0, 60.0, 30.0, 10.0, 8.0, 5.0};
  BaseDrive d;
  Requirements req;
  const Budget bud = payload_budget(m, b, q, masses(), arm_model(m), limits, 5.0, d, req);
  rec("reach_m", 0.65, 2);
  rec("arm_kg", bud.arm, 1);
  rec("tip_kg", bud.tip, 1);
  rec("trac_kg", bud.traction, 1);
  rec("v_stop", req.stop_speed, 1);
  rec("d_stop", req.stop_dist, 2);
  rec("a_req", req.stop_speed * req.stop_speed / (2.0 * req.stop_dist), 2);
  rec("a_want", req.accel_wanted, 1);
  rec("best_kg", bud.best, 1);
  rec("binding", bud.binding, 0);
  RC_CHECK(bud.arm > 0.0);
  RC_CHECK(bud.tip > 0.0);
  RC_CHECK(bud.traction > 0.0);
  const double least = std::fmin(bud.arm, std::fmin(bud.tip, bud.traction));
  RC_CHECK_NEAR(bud.best, least, 1e-12);
  const double values[3] = {bud.arm, bud.tip, bud.traction};
  RC_CHECK_NEAR(values[bud.binding], least, 1e-12);
  // a gentler stop is a heavier load for the tipping limit
  Requirements gentle = req;
  gentle.stop_dist = 0.6;
  const Budget g = payload_budget(m, b, q, masses(), arm_model(m), limits, 5.0, d, gentle);
  RC_CHECK(g.tip > bud.tip);
  // a friction that cannot give the wanted acceleration gives no traction budget
  BaseDrive slick = d;
  slick.mu = 0.03;
  RC_CHECK(payload_budget(m, b, q, masses(), arm_model(m), limits, 5.0, slick, req).traction < 0.0);
}
