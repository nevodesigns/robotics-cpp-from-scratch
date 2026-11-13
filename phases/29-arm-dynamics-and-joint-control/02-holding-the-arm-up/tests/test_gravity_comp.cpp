#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "rc/arm/ik_closed.hpp"
#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
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

// Gains for a joint each: stiffness chosen, damping critical for the inertia the joint sees at the target pose (the diagonal of the mass matrix).
PdGains gains(const ArmModel& m, const std::vector<double>& at) {
  PdGains g;
  const double kp[6] = {400.0, 300.0, 200.0, 80.0, 60.0, 40.0};
  const std::vector<std::vector<double> > mm = mass_matrix(m, at);
  for (std::size_t i = 0; i < 6; ++i) {
    g.kp.push_back(kp[i]);
    g.kd.push_back(2.0 * std::sqrt(kp[i] * mm[i][i]));
  }
  return g;
}

std::vector<double> target() {
  std::vector<double> q(6);
  q[0] = 0.3;
  q[1] = 0.5;
  q[2] = 0.4;
  q[3] = 0.2;
  q[4] = 0.8;
  q[5] = -0.3;
  return q;
}

}  // namespace

RC_TEST("a PD controller that has to hold the arm up with its error sags by the torque over the gain, and adding gravity takes it away") {
  const ArmModel m = typical();
  const PdGains g = gains(m, target());
  const std::vector<double> tg = gravity_torques(m, target());
  // The shoulder (joint 2) carries the most.
  const double pred = std::fabs(tg[1]) / g.kp[1];
  const double droop = settle(m, m, target(), target(), g, false, 6.0, 0.002);
  const double comp = settle(m, m, target(), target(), g, true, 6.0, 0.002);
  rec("pred_droop", pred, 4);
  rec("droop", droop, 4);
  rec("comp_err", comp, 9);
  RC_CHECK(droop > 0.01);
  RC_CHECK(droop > 0.8 * pred && droop < 1.6 * pred);
  RC_CHECK(comp < 1e-6);
}

RC_TEST("a payload the controller does not know about is a droop, and its mass is found from the torques in six poses") {
  const ArmModel m = typical();
  const double at_tool[3] = {0.0, 0.0, 0.10};
  const ArmModel loaded = with_payload(m, 2.0, at_tool);
  const PdGains g = gains(m, target());
  const double pay_err = settle(loaded, m, target(), target(), g, true, 6.0, 0.002);
  rec("pay_err", pay_err, 4);
  // Six poses held still, each measured with 0.1 N m of noise: what the arm needs less what the arm alone needs.
  std::vector<std::vector<double> > poses, extra;
  unsigned long long s = 5;
  for (int k = 0; k < 6; ++k) {
    std::vector<double> q(6);
    for (int i = 0; i < 6; ++i) q[static_cast<std::size_t>(i)] = 0.7 * std::sin(1.9 * (i + 1) + 1.3 * k);
    poses.push_back(q);
    std::vector<double> need = gravity_torques(loaded, q);
    const std::vector<double> base = gravity_torques(m, q);
    for (std::size_t i = 0; i < 6; ++i) {
      s = s * 6364136223846793005ULL + 1442695040888963407ULL;
      const double noise = 0.1 * (static_cast<double>((s >> 33) & 0xFFFFFFULL) / 16777216.0 * 2.0 - 1.0);
      need[i] = need[i] - base[i] + noise;
    }
    extra.push_back(need);
  }
  const double est = estimate_payload(m.table, trans(0.0, 0.0, 0.10), poses, extra);
  rec("est_mass", est, 2);
  RC_CHECK(pay_err > 0.01);
  RC_CHECK_NEAR(est, 2.0, 0.1);
  const ArmModel known = with_payload(m, est, at_tool);
  const double fixed = settle(loaded, known, target(), target(), g, true, 6.0, 0.002);
  rec("fixed_err", fixed, 5);
  RC_CHECK(fixed < 0.2 * pay_err);
}

RC_TEST("a payload makes the pair heavier, moves the centre of mass towards it, and the torque to hold it is the Jacobian's transpose") {
  const ArmModel m = typical();
  const double at_tool[3] = {0.0, 0.0, 0.10};
  const ArmModel loaded = with_payload(m, 2.0, at_tool);
  RC_CHECK_NEAR(loaded.links.back().mass, m.links.back().mass + 2.0, 1e-12);
  RC_CHECK(loaded.links.back().com[2] > m.links.back().com[2]);
  const std::vector<double> q = target();
  const std::vector<double> diff_g = gravity_torques(loaded, q);
  const std::vector<double> base_g = gravity_torques(m, q);
  const std::vector<double> pay = payload_torque(m.table, q, trans(0.0, 0.0, 0.10), 2.0);
  for (std::size_t i = 0; i < 6; ++i) RC_CHECK_NEAR(diff_g[i] - base_g[i], pay[i], 1e-9);
}

RC_TEST("with nothing in the gripper the estimate is zero") {
  const ArmModel m = typical();
  std::vector<std::vector<double> > poses, extra;
  for (int k = 0; k < 4; ++k) {
    std::vector<double> q(6, 0.1 * (k + 1));
    poses.push_back(q);
    extra.push_back(std::vector<double>(6, 0.0));
  }
  RC_CHECK_NEAR(estimate_payload(m.table, trans(0.0, 0.0, 0.10), poses, extra), 0.0, 1e-12);
}
