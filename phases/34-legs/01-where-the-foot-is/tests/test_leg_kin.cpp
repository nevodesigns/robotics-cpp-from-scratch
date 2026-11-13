#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


RC_TEST("the foot goes back to where it was asked, for either knee and either side") {
  int n = 0, ok = 0;
  double worst = 0.0, worst_knee = 0.0;
  for (int side = -1; side <= 1; side += 2) {
    LegGeom g;
    g.side = side;
    for (double x = -0.15; x <= 0.15 + 1e-9; x += 0.05)
      for (double y = -0.20; y <= 0.20 + 1e-9; y += 0.05)
        for (double z = -0.40; z <= -0.15 + 1e-9; z += 0.05) {
          ++n;
          double qa[3], qb[3];
          const bool a = leg_ik(g, x, y, z, 1, qa), b = leg_ik(g, x, y, z, -1, qb);
          RC_CHECK(a == b);
          if (!a) continue;
          ++ok;
          const FootPos pa = leg_fk(g, qa[0], qa[1], qa[2]), pb = leg_fk(g, qb[0], qb[1], qb[2]);
          worst = std::fmax(worst, std::sqrt((pa.x - x) * (pa.x - x) + (pa.y - y) * (pa.y - y) + (pa.z - z) * (pa.z - z)));
          worst_knee = std::fmax(worst_knee, std::sqrt((pa.x - pb.x) * (pa.x - pb.x) + (pa.y - pb.y) * (pa.y - pb.y) + (pa.z - pb.z) * (pa.z - pb.z)));
        }
  }
  LegGeom g0;
  rec("l1", g0.l1, 2);
  rec("l2", g0.l2, 2);
  rec("l0", g0.l0, 2);
  rec("n_grid", n, 0);
  rec("n_ok", ok, 0);
  rec("err", worst, 9);
  rec("err_knee", worst_knee, 9);
  RC_CHECK(ok > n / 4 && ok < n);
  RC_CHECK(worst < 1e-9 && worst_knee < 1e-9);
}

RC_TEST("a standing leg has the knee bent and its foot below the hip, and a place too far is refused") {
  LegGeom g;
  double q[3];
  RC_CHECK(leg_ik(g, 0.0, g.l0, -0.30, -1, q));
  rec("drop", 0.30, 2);
  rec("knee_deg", q[2] * 180.0 / 3.14159265358979323846, 0);
  RC_CHECK(std::fabs(q[0]) < 1e-9);          // straight below the hip: no lean
  RC_CHECK(q[2] < 0.0 && q[1] > 0.0);        // the knee bent back, the thigh forward
  const FootPos p = leg_fk(g, q[0], q[1], q[2]);
  RC_CHECK_NEAR(p.z, -0.30, 1e-12);
  double q2[3];
  rec("far", 0.50, 2);
  RC_CHECK(!leg_ik(g, 0.0, g.l0, -0.50, -1, q2));
  RC_CHECK(!leg_ik(g, 0.0, 0.0, -0.05, -1, q2));  // closer to the lean axis than the offset
}

RC_TEST("the leg is straight at full stretch and a leg to its full length reaches the longest") {
  LegGeom g;
  double q[3];
  const double reach = g.l1 + g.l2;
  RC_CHECK(leg_ik(g, 0.0, g.l0, -reach + 1e-9, 1, q));
  rec("str_out", straightness(q[2]), 3);
  rec("l_max", reach, 2);
  RC_CHECK(straightness(q[2]) < 1e-3);
  RC_CHECK(straightness(1.5707963267948966) > 0.999);
  const FootPos p = leg_fk(g, 0.0, 0.0, 0.0);
  RC_CHECK_NEAR(p.z, -reach, 1e-12);
  RC_CHECK_NEAR(p.x, 0.0, 1e-12);
}
