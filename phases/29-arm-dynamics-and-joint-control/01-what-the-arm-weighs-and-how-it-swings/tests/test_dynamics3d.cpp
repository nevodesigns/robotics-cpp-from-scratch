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
    {"hold", 3.9240000000000004},
    {"hold90", 2.4027570199271073e-16},
    {"grav_err", 5.529077640176183e-09},
    {"sym", 1.1102230246251565e-16},
    {"pd_n", 60.0},
    {"quad_err", 0.0},
    {"drift_rk4", 4.963182368245614e-10},
    {"drift_euler", 0.03723060577635782},
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

// A typical arm: the elbow arm's table with masses of a small arm (shoulder 3 kg, upper arm 4 kg, elbow 2 kg, forearm 2 kg, wrist 1 kg, flange 0.5 kg).
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

std::vector<double> some_q(int k) {
  std::vector<double> q(6);
  for (int i = 0; i < 6; ++i) q[static_cast<std::size_t>(i)] = 0.8 * std::sin(1.3 * (i + 1) + 0.7 * k);
  return q;
}

}  // namespace

RC_TEST("a rod out horizontally from a horizontal axis needs m g L / 2 to be held, and nothing when it stands up") {
  ArmModel m;
  JointRow r0, r1;
  r0.alpha = kPi / 2.0;  // joint 1 vertical
  r1.a = 0.4;            // joint 2 horizontal, a rod of 0.4 m
  m.table.rows.push_back(r0);
  m.table.rows.push_back(r1);
  std::vector<double> masses(2);
  masses[0] = 0.0;
  masses[1] = 2.0;
  m.links = rod_links(m.table, masses, 0.01);
  std::vector<double> q(2, 0.0);
  const double hold = gravity_torques(m, q)[1];
  q[1] = kPi / 2.0;
  const double hold90 = gravity_torques(m, q)[1];
  rec("hold", hold, 3);
  rec("hold90", hold90, 6);
  RC_CHECK_NEAR(hold, 2.0 * 9.81 * 0.4 / 2.0, 1e-9);
  RC_CHECK_NEAR(hold90, 0.0, 1e-9);
  q[1] = 0.6;
  RC_CHECK_NEAR(gravity_torques(m, q)[1], 2.0 * 9.81 * 0.2 * std::cos(0.6), 1e-9);
}

RC_TEST("the torque of gravity is the slope of the potential energy, and the mass matrix is symmetric and positive") {
  const ArmModel m = typical();
  double worst = 0.0, asym = 0.0;
  for (int k = 0; k < 10; ++k) {
    const std::vector<double> q = some_q(k);
    const std::vector<double> tg = gravity_torques(m, q);
    for (std::size_t j = 0; j < 6; ++j) {
      std::vector<double> a = q, b = q;
      a[j] += 1e-6;
      b[j] -= 1e-6;
      const double slope = (energy(m, a, std::vector<double>(6, 0.0)) - energy(m, b, std::vector<double>(6, 0.0))) / 2e-6;
      worst = std::fmax(worst, std::fabs(slope - tg[j]));
    }
    const std::vector<std::vector<double> > mm = mass_matrix(m, q);
    for (std::size_t i = 0; i < 6; ++i)
      for (std::size_t j = 0; j < 6; ++j) asym = std::fmax(asym, std::fabs(mm[i][j] - mm[j][i]));
  }
  rec("grav_err", worst, 8);
  rec("sym", asym, 12);
  RC_CHECK(worst < 1e-5);
  RC_CHECK(asym < 1e-9);
  const std::vector<std::vector<double> > mm = mass_matrix(m, some_q(3));
  int pd = 0;
  const int n = 60;
  for (int k = 0; k < n; ++k) {
    double x[6], s = 0.0;
    for (int i = 0; i < 6; ++i) x[i] = std::sin(2.9 * (i + 1) + 1.7 * k);
    for (int i = 0; i < 6; ++i)
      for (int j = 0; j < 6; ++j) s += x[i] * mm[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] * x[j];
    if (s > 0.0) ++pd;
  }
  rec("pd_n", n, 0);
  RC_CHECK(pd == n);
}

RC_TEST("what the joints throw at each other grows with the square of the speed") {
  const ArmModel m = typical();
  const std::vector<double> q = some_q(2);
  std::vector<double> qd(6), qd2(6);
  for (int i = 0; i < 6; ++i) {
    qd[static_cast<std::size_t>(i)] = 0.5 * std::cos(1.1 * (i + 1));
    qd2[static_cast<std::size_t>(i)] = 2.0 * qd[static_cast<std::size_t>(i)];
  }
  const std::vector<double> a = velocity_torques(m, q, qd), b = velocity_torques(m, q, qd2);
  double worst = 0.0, size = 0.0;
  for (std::size_t i = 0; i < 6; ++i) {
    worst = std::fmax(worst, std::fabs(b[i] - 4.0 * a[i]));
    size = std::fmax(size, std::fabs(a[i]));
  }
  rec("quad_err", worst, 12);
  RC_CHECK(size > 0.01);
  RC_CHECK(worst < 1e-9);
}

RC_TEST("an arm let fall keeps its energy with a fourth order step and does not with a first order one") {
  const ArmModel m = typical();
  std::vector<double> q0 = some_q(1), qd0(6, 0.0);
  const double e0 = energy(m, q0, qd0);
  double drift[2];
  for (int mode = 0; mode < 2; ++mode) {
    std::vector<double> q = q0, qd = qd0;
    for (int s = 0; s < 1000; ++s) step_dynamics(m, q, qd, std::vector<double>(6, 0.0), 0.001, mode == 0);
    drift[mode] = std::fabs(energy(m, q, qd) - e0) / std::fabs(e0);
  }
  rec("drift_rk4", drift[0], 12);
  rec("drift_euler", drift[1], 5);
  RC_CHECK(drift[0] < 1e-6);
  RC_CHECK(drift[1] > 20.0 * drift[0]);
}

RC_TEST("held at the torque of gravity the arm does not move") {
  const ArmModel m = typical();
  const std::vector<double> q = some_q(4);
  const std::vector<double> a = forward_dynamics(m, q, std::vector<double>(6, 0.0), gravity_torques(m, q));
  for (std::size_t i = 0; i < 6; ++i) RC_CHECK_NEAR(a[i], 0.0, 1e-9);
}
