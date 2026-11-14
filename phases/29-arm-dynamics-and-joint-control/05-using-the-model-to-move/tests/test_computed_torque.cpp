#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

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
    {"rms_pdg", 0.04293036132256803},
    {"rms_ct", 5.6948097690533645e-05},
    {"tq_pdg", 28.4502694401661},
    {"tq_ct", 28.4502694401661},
    {"crossings", 0.0},
    {"settle", 0.334},
    {"rms_10", 0.00466944347617554},
    {"rms_20", 0.008526413941359166},
    {"rms_40", 0.01455426221699776},
    {"rms_light", 0.034686158477475276},
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

ArmModel typical() {
  ArmModel m;
  m.table = elbow_arm();
  std::vector<double> masses(6);
  masses[0] = 3.0;
  masses[1] = 4.0;
  masses[2] = 2.0;
  masses[3] = 2.0;
  masses[4] = 1.0;
  masses[5] = 0.5;
  m.links = rod_links(m.table, masses, 0.04);
  return m;
}

Quintic the_move() {
  Quintic mv;
  mv.q0.assign(6, 0.0);
  mv.q1.assign(6, 0.0);
  const double a[6] = {0.7, -0.6, 0.5, 0.7, -0.6, 0.7};
  for (int i = 0; i < 6; ++i) {
    mv.q0[static_cast<std::size_t>(i)] = 0.1 * (i - 2);
    mv.q1[static_cast<std::size_t>(i)] = mv.q0[static_cast<std::size_t>(i)] + a[i];
  }
  mv.duration = 1.0;
  return mv;
}

PdGains pd_gains(const ArmModel& m, const std::vector<double>& at) {
  PdGains g;
  const double kp[6] = {400.0, 300.0, 200.0, 80.0, 60.0, 40.0};
  const std::vector<std::vector<double> > mm = mass_matrix(m, at);
  for (std::size_t i = 0; i < 6; ++i) {
    g.kp.push_back(kp[i]);
    g.kd.push_back(2.0 * std::sqrt(kp[i] * mm[i][i]));
  }
  return g;
}

}  // namespace

RC_TEST("the quintic starts and ends at rest, with the right end points, and its derivatives are consistent") {
  const Quintic mv = the_move();
  std::vector<double> q, qd, qdd;
  quintic_at(mv, 0.0, q, qd, qdd);
  for (std::size_t i = 0; i < 6; ++i) {
    RC_CHECK_NEAR(q[i], mv.q0[i], 1e-12);
    RC_CHECK_NEAR(qd[i], 0.0, 1e-12);
    RC_CHECK_NEAR(qdd[i], 0.0, 1e-12);
  }
  quintic_at(mv, 1.0, q, qd, qdd);
  for (std::size_t i = 0; i < 6; ++i) {
    RC_CHECK_NEAR(q[i], mv.q1[i], 1e-12);
    RC_CHECK_NEAR(qd[i], 0.0, 1e-12);
  }
  std::vector<double> qa, qb, d1, d2, a1, a2;
  quintic_at(mv, 0.4999, qa, d1, a1);
  quintic_at(mv, 0.5001, qb, d2, a2);
  for (std::size_t i = 0; i < 6; ++i) {
    RC_CHECK_NEAR((qb[i] - qa[i]) / 0.0002, 0.5 * (d1[i] + d2[i]), 1e-5);
    RC_CHECK_NEAR((d2[i] - d1[i]) / 0.0002, 0.5 * (a1[i] + a2[i]), 1e-4);
  }
}

RC_TEST("with an exact model computed torque follows a fast move far more closely than a PD controller with gravity") {
  const ArmModel m = typical();
  const Quintic mv = the_move();
  const PdGains pd = pd_gains(m, mv.q0);
  const TrackResult a = track(m, m, mv, 0, pd, 0.0, 0.002, 1.6);
  const TrackResult b = track(m, m, mv, 1, pd, 20.0, 0.002, 1.6);
  rec("rms_pdg", a.rms_error, 5);
  rec("rms_ct", b.rms_error, 9);
  rec("tq_pdg", a.max_torque, 1);
  rec("tq_ct", b.max_torque, 1);
  RC_CHECK(a.rms_error > 1e-3);
  RC_CHECK(b.rms_error < a.rms_error / 100.0);
}

RC_TEST("from a start out of place the error of computed torque dies away as a critically damped second order system") {
  const ArmModel m = typical();
  Quintic hold;
  hold.q0.assign(6, 0.3);
  hold.q1.assign(6, 0.3);
  hold.duration = 1.0;
  std::vector<double> q = hold.q0, qd(6, 0.0);
  q[1] += 0.1;
  const double wn = 20.0, dt = 0.002;
  int crossings = 0;
  double settle = -1.0, prev = 0.1;
  for (int k = 0; k < 1000; ++k) {
    const std::vector<double> z(6, 0.0);
    const std::vector<double> tau = computed_torque(m, q, qd, hold.q0, z, z, wn * wn, 2.0 * wn);
    step_dynamics(m, q, qd, tau, dt, true);
    const double e = q[1] - 0.3;
    if (prev * e < 0.0) ++crossings;
    prev = e;
    if (settle < 0.0 && std::fabs(e) < 0.001) settle = (k + 1) * dt;
  }
  rec("crossings", crossings, 0);
  rec("settle", settle, 2);
  RC_CHECK(crossings == 0);
  // 1 percent of 0.1 rad for a critically damped system of 20 rad/s: about 0.33 s.
  RC_CHECK(settle > 0.2 && settle < 0.5);
}

RC_TEST("a model that is wrong gives an error in proportion, and one that is too light is worse than one that is too heavy by as much") {
  const ArmModel m = typical();
  const Quintic mv = the_move();
  const PdGains pd = pd_gains(m, mv.q0);
  const double e10 = track(m, scaled_model(m, 1.1), mv, 1, pd, 20.0, 0.002, 1.6).rms_error;
  const double e20 = track(m, scaled_model(m, 1.2), mv, 1, pd, 20.0, 0.002, 1.6).rms_error;
  const double e40 = track(m, scaled_model(m, 1.4), mv, 1, pd, 20.0, 0.002, 1.6).rms_error;
  const double light = track(m, scaled_model(m, 0.6), mv, 1, pd, 20.0, 0.002, 1.6).rms_error;
  const double pdg = track(m, m, mv, 0, pd, 0.0, 0.002, 1.6).rms_error;
  rec("rms_10", e10, 5);
  rec("rms_20", e20, 5);
  rec("rms_40", e40, 5);
  rec("rms_light", light, 5);
  RC_CHECK(e10 < e20 && e20 < e40);
  RC_CHECK(e10 < pdg);
  RC_CHECK(light > e40);
}
