#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}


namespace {

double biggest_gap(const Tf3& a, const Tf3& b) {
  double m = 0.0;
  for (int i = 0; i < 9; ++i) m = std::max(m, std::fabs(a.r[i] - b.r[i]));
  for (int i = 0; i < 3; ++i) m = std::max(m, std::fabs(a.t[i] - b.t[i]));
  return m;
}

}  // namespace

RC_TEST("a transform composed with its inverse is the identity, in the plane and in space") {
  Tf2 a;
  a.x = 1.5;
  a.y = -2.0;
  a.theta = 2.4;
  const Tf2 id = compose(a, inverse(a));
  RC_CHECK_NEAR(id.x, 0.0, 1e-12);
  RC_CHECK_NEAR(id.y, 0.0, 1e-12);
  RC_CHECK_NEAR(id.theta, 0.0, 1e-12);

  const Tf3 t = from_rpy(1.0, 2.0, 3.0, 0.3, -0.4, 0.9);
  const Tf3 one = compose(t, inverse(t));
  const double gap = biggest_gap(one, Tf3());
  rec("identity_gap", gap * 1e15, 1);
  RC_CHECK(gap < 1e-12);
}

RC_TEST("the order of composition matters, by a measurable amount") {
  const Tf3 shift = from_rpy(1, 0, 0, 0, 0, 0);
  const Tf3 turn = from_rpy(0, 0, 0, 0, 0, 1.5707963267948966);
  Point3 p;
  p.x = 1.0;
  const Point3 u = apply(compose(shift, turn), p);
  const Point3 v = apply(compose(turn, shift), p);
  const double gap = std::sqrt((u.x - v.x) * (u.x - v.x) + (u.y - v.y) * (u.y - v.y));
  rec("order_gap", gap);
  RC_CHECK_NEAR(u.x, 1.0, 1e-9);
  RC_CHECK_NEAR(u.y, 1.0, 1e-9);
  RC_CHECK_NEAR(v.y, 2.0, 1e-9);
  RC_CHECK_NEAR(gap, std::sqrt(2.0), 1e-9);
}

RC_TEST("the chain map, odom, base, lidar carries a point to the map, and agrees with the plane arithmetic") {
  FrameTree tree;
  tree.add("odom", "map", from_rpy(0.5, 0.2, 0.0, 0, 0, 0.1));
  tree.add("base", "odom", from_rpy(3.0, 1.0, 0.0, 0, 0, 0.7));
  tree.add("lidar", "base", from_rpy(0.30, 0.0, 0.25, 0, 0, 0));
  Tf3 lidar_to_map;
  RC_CHECK(tree.between("lidar", "map", lidar_to_map));
  Point3 p;
  p.x = 4.0;
  const Point3 m = apply(lidar_to_map, p);
  rec("mx", m.x);
  rec("my", m.y);

  Tf2 a;
  a.x = 0.5;
  a.y = 0.2;
  a.theta = 0.1;
  Tf2 b;
  b.x = 3.0;
  b.y = 1.0;
  b.theta = 0.7;
  Tf2 c;
  c.x = 0.30;
  const Tf2 whole = compose(a, compose(b, c));
  const double mx = whole.x + std::cos(whole.theta) * 4.0;
  const double my = whole.y + std::sin(whole.theta) * 4.0;
  RC_CHECK_NEAR(m.x, mx, 1e-9);
  RC_CHECK_NEAR(m.y, my, 1e-9);

  Tf3 back;
  RC_CHECK(tree.between("map", "lidar", back));
  RC_CHECK(biggest_gap(compose(back, lidar_to_map), Tf3()) < 1e-12);
  RC_CHECK(tree.update("base", from_rpy(5.0, 1.0, 0.0, 0, 0, 0.7)));
  Tf3 moved;
  RC_CHECK(tree.between("lidar", "map", moved));
  rec("moved", moved.t[0] - lidar_to_map.t[0]);
  RC_CHECK_NEAR(moved.t[0] - lidar_to_map.t[0], 2.0 * std::cos(0.1), 1e-9);
}

RC_TEST("a lidar tipped down sees the ground at a range that is not its height divided by anything simple") {
  FrameTree tree;
  const double tilt = 5.0 * 3.141592653589793 / 180.0;
  tree.add("lidar", "base", from_rpy(0.0, 0.0, 0.25, 0, tilt, 0));
  Tf3 to_base;
  RC_CHECK(tree.between("lidar", "base", to_base));
  Point3 near_pt;
  near_pt.x = 1.0;
  Point3 far_pt;
  far_pt.x = 3.0;
  const Point3 n = apply(to_base, near_pt);
  const Point3 f = apply(to_base, far_pt);
  const double along = -n.z / ((f.z - n.z) / 2.0) + 1.0;
  const Point3 hit = apply(to_base, Point3{along, 0.0, 0.0});
  rec("ground_range", hit.x);
  RC_CHECK_NEAR(hit.z, 0.0, 1e-9);
  RC_CHECK_NEAR(hit.x, 0.25 / std::tan(tilt), 1e-9);
  RC_CHECK(hit.x < 3.0);
}

RC_TEST("an unknown frame, two separate trees and a loop all give no transform instead of a wrong one") {
  FrameTree tree;
  tree.add("base", "odom", Tf3());
  tree.add("other", "elsewhere", Tf3());
  Tf3 out;
  RC_CHECK(!tree.between("base", "nowhere", out));
  RC_CHECK(!tree.between("base", "other", out));
  tree.add("a", "b", Tf3());
  tree.add("b", "a", Tf3());
  RC_CHECK(!tree.between("a", "base", out));
  RC_CHECK(tree.between("base", "base", out));
}
