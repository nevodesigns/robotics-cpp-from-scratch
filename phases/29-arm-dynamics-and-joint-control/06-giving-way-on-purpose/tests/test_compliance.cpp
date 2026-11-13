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

// The tool pointing down over a point, and the joint angles that put it there (the solution nearest to the straight arm).
Pose down_at(double x, double y, double z) {
  Pose p;
  p.r[1][1] = -1.0;
  p.r[2][2] = -1.0;
  p.p[0] = x;
  p.p[1] = y;
  p.p[2] = z;
  return p;
}

std::vector<double> nearest_solution(const ArmTable& t, const Pose& target) {
  const std::vector<std::vector<double> > sols = ik_spherical(t, target, Pose(), Pose());
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

}  // namespace

RC_TEST("the tool settles on the force of the stiffness times how far the target is below the surface") {
  const ArmModel m = typical();
  Surface s;
  s.z = 0.25;
  const std::vector<double> q0 = nearest_solution(m.table, down_at(0.5, 0.0, 0.30));
  const Pose target = down_at(0.5, 0.0, 0.23);
  const double ks[3] = {200.0, 500.0, 1000.0};
  double f[3];
  for (int i = 0; i < 3; ++i) {
    const double dz = 2.0 * std::sqrt(ks[i] * 4.0);
    f[i] = press(m, q0, target, Pose(), ks[i], dz, s, 1.5, 0.0005).settled_force;
  }
  rec("f200", f[0], 2);
  rec("f500", f[1], 2);
  rec("f1000", f[2], 2);
  RC_CHECK_NEAR(f[0], 200.0 * 0.02, 0.5);
  RC_CHECK_NEAR(f[1], 500.0 * 0.02, 0.8);
  RC_CHECK_NEAR(f[2], 1000.0 * 0.02, 1.5);
}

RC_TEST("a softer arm meets the surface with a smaller peak force") {
  const ArmModel m = typical();
  Surface s;
  s.z = 0.25;
  const std::vector<double> q0 = nearest_solution(m.table, down_at(0.5, 0.0, 0.30));
  const Pose target = down_at(0.5, 0.0, 0.23);
  const PressResult soft = press(m, q0, target, Pose(), 200.0, 2.0 * std::sqrt(200.0 * 4.0), s, 2.5, 0.0005);
  const PressResult stiff = press(m, q0, target, Pose(), 2000.0, 2.0 * std::sqrt(2000.0 * 4.0), s, 2.5, 0.0005);
  rec("peak_soft", soft.peak_force, 1);
  rec("peak_stiff", stiff.peak_force, 1);
  RC_CHECK(stiff.peak_force > 1.5 * soft.peak_force);
  RC_CHECK(soft.steady && stiff.steady);
}

RC_TEST("in free space the impedance holds the pose against gravity, and a target to one side is reached") {
  const ArmModel m = typical();
  const std::vector<double> q0 = nearest_solution(m.table, down_at(0.5, 0.0, 0.30));
  Surface none;
  none.z = -10.0;
  const PressResult hold = press(m, q0, down_at(0.5, 0.0, 0.30), Pose(), 500.0, 90.0, none, 1.0, 0.0005);
  RC_CHECK(hold.peak_force == 0.0);
  // The torque of an arm at the target is the torque of gravity when the target is where the tool is.
  const double stiff[6] = {500, 500, 500, 10, 10, 10}, damp[6] = {90, 90, 90, 0.5, 0.5, 0.5};
  const std::vector<double> tau = impedance_torque(m, q0, std::vector<double>(6, 0.0), forward(m.table, q0, Pose(), Pose()), Pose(), stiff, damp);
  const std::vector<double> tg = gravity_torques(m, q0);
  for (std::size_t i = 0; i < 6; ++i) RC_CHECK_NEAR(tau[i], tg[i], 1e-9);
}

RC_TEST("an admittance with too little damper rings on a stiff surface and is steady with enough, and is fine on a soft one") {
  Surface stiff, soft;
  stiff.z = soft.z = 0.25;
  soft.k = 500.0;
  AdmittanceConfig a;
  a.dv = 100.0;
  const PressResult bad = admittance_press(a, stiff, 6.0);
  a.dv = 1500.0;
  const PressResult good = admittance_press(a, stiff, 6.0);
  a.dv = 100.0;
  const PressResult on_soft = admittance_press(a, soft, 6.0);
  rec("adm_bad", bad.steady ? 1 : 0, 0);
  rec("adm_good", good.steady ? 1 : 0, 0);
  rec("adm_force", good.settled_force, 1);
  rec("adm_soft", on_soft.steady ? 1 : 0, 0);
  RC_CHECK(!bad.steady);
  RC_CHECK(good.steady);
  RC_CHECK(on_soft.steady);
  RC_CHECK_NEAR(good.settled_force, 500.0 * 0.02, 1.5);
}
