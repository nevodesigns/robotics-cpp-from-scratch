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
    {"h", 0.3},
    {"mass", 12.0},
    {"tk_stand", 4.325308987806535},
    {"tk_trot", 8.65061797561307},
    {"rms_trot", 6.120587854119895},
    {"rise_stand", 14.031223379999995},
    {"rise_trot", 28.096196759999984},
    {"limit", 60.0},
    {"h_low", 0.2},
    {"rise_low", 44.336038634999994},
    {"t_limit", 25.14803255514769},
    {"m_max", 17.541709894296893},
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

JointMotor leg_motor() {
  JointMotor m;
  m.kt_out = 1.0;        // N m per ampere at the output shaft (typical of a quadruped's joint)
  m.resistance = 0.5;
  m.r_th = 1.5;
  m.c_th = 60.0;
  m.rise_limit = 60.0;
  m.tau_peak = 33.0;
  m.tau_cont = 10.0;
  return m;
}

}  // namespace

RC_TEST("the stance torque is the weight's share through the Jacobian, and two feet down give twice four") {
  LegGeom g;
  double t4[3], t2[3];
  stance_torques(g, 0.30, 12.0, 4, t4);
  stance_torques(g, 0.30, 12.0, 2, t2);
  RC_CHECK_NEAR(t2[2], 2.0 * t4[2], 1e-9);
  const double w = 12.0 * 9.81 / 4.0;
  double q[3];
  leg_ik(g, 0.0, g.l0, -0.30, -1, q);
  RC_CHECK_NEAR(std::fabs(t4[2]), w * g.l2 * std::fabs(std::sin(q[1] + q[2])), 1e-9);
}

RC_TEST("the rms torque over a cycle lies between the swing and the stance torque and heat goes as its square") {
  RC_CHECK_NEAR(cycle_rms_torque(10.0, 0.0, 0.5), 10.0 * std::sqrt(0.5), 1e-12);
  RC_CHECK_NEAR(cycle_rms_torque(10.0, 2.0, 1.0), 10.0, 1e-12);
  RC_CHECK_NEAR(cycle_rms_torque(10.0, 2.0, 0.0), 2.0, 1e-12);
  RC_CHECK(cycle_rms_torque(10.0, 2.0, 0.6) > 2.0 && cycle_rms_torque(10.0, 2.0, 0.6) < 10.0);
  const JointMotor m = leg_motor();
  RC_CHECK_NEAR(winding_rise(m, 6.0) / winding_rise(m, 3.0), 4.0, 1e-9);
}

RC_TEST("a trot heats the knees more than standing, a crouch more still, and the legs can carry a heaviest body") {
  LegGeom g;
  const JointMotor m = leg_motor();
  const double mass = 12.0, h = 0.30, h_low = 0.20, swing = 0.3;
  double ts[3], tt[3], tl[3];
  stance_torques(g, h, mass, 4, ts);
  stance_torques(g, h, mass, 2, tt);
  stance_torques(g, h_low, mass, 2, tl);
  const double rms_trot = cycle_rms_torque(std::fabs(tt[2]), swing, 0.5);
  const double rms_low = cycle_rms_torque(std::fabs(tl[2]), swing, 0.5);
  const double rise_stand = winding_rise(m, std::fabs(ts[2])), rise_trot = winding_rise(m, rms_trot), rise_low = winding_rise(m, rms_low);
  rec("h", h, 2);
  rec("mass", mass, 0);
  rec("tk_stand", std::fabs(ts[2]), 2);
  rec("tk_trot", std::fabs(tt[2]), 2);
  rec("rms_trot", rms_trot, 2);
  rec("rise_stand", rise_stand, 1);
  rec("rise_trot", rise_trot, 1);
  rec("limit", m.rise_limit, 0);
  rec("h_low", h_low, 2);
  rec("rise_low", rise_low, 1);
  RC_CHECK(rise_trot > rise_stand);
  RC_CHECK(rise_low > rise_trot);
  // a crouch held at the torque of a heavy robot reaches the limit in a time
  const double heavy = 40.0;
  double th[3];
  stance_torques(g, h_low, heavy, 4, th);
  const double t_limit = time_to_limit(m, std::fabs(th[2]));
  rec("t_limit", t_limit < 1e17 ? t_limit : 0.0, 0);
  RC_CHECK(t_limit < 1e17 || winding_rise(m, std::fabs(th[2])) <= m.rise_limit);
  const double mmax = max_body_mass(g, m, h, 2, 0.5, swing);
  rec("m_max", mmax, 1);
  double tm[3];
  stance_torques(g, h, mmax, 2, tm);
  RC_CHECK_NEAR(winding_rise(m, cycle_rms_torque(std::fabs(tm[2]), swing, 0.5)), m.rise_limit, 0.01);
  RC_CHECK(mmax > mass);
  RC_CHECK(max_body_mass(g, m, h_low, 2, 0.5, swing) < mmax);
}
