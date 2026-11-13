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

Sphere sphere(double x, double y, double z, double r) {
  Sphere s;
  s.c[0] = x;
  s.c[1] = y;
  s.c[2] = z;
  s.r = r;
  return s;
}

std::vector<double> radii() {
  std::vector<double> r(6, 0.05);
  r[0] = 0.08;
  r[1] = 0.06;
  r[3] = 0.05;
  r[5] = 0.04;
  return r;
}

std::vector<double> q_down_at(const ArmTable& t, double x, double y, double z) {
  Pose p;
  p.r[1][1] = -1.0;
  p.r[2][2] = -1.0;
  p.p[0] = x;
  p.p[1] = y;
  p.p[2] = z;
  const std::vector<std::vector<double> > sols = ik_spherical(t, p, Pose(), trans(0.0, 0.0, 0.10));
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

RC_TEST("distances between segments, a capsule and a sphere, and a segment and a box are what a ruler gives") {
  const double a1[3] = {-0.5, 0.0, 0.3}, b1[3] = {0.5, 0.0, 0.3}, a2[3] = {0.0, -0.5, 0.0}, b2[3] = {0.0, 0.5, 0.0};
  rec("ss", seg_seg_distance(a1, b1, a2, b2), 3);
  RC_CHECK_NEAR(seg_seg_distance(a1, b1, a2, b2), 0.3, 1e-12);
  // Parallel, and end to end.
  const double a3[3] = {-0.5, 0.2, 0.3};
  const double b3[3] = {0.5, 0.2, 0.3};
  RC_CHECK_NEAR(seg_seg_distance(a1, b1, a3, b3), 0.2, 1e-12);
  const double c0[3] = {1.0, 0.0, 0.0}, c1[3] = {2.0, 0.0, 0.0}, d0[3] = {-1.0, 0.0, 0.0}, d1[3] = {0.0, 0.0, 0.0};
  RC_CHECK_NEAR(seg_seg_distance(c0, c1, d0, d1), 1.0, 1e-12);
  Capsule cap;
  cap.a[0] = -0.5; cap.a[1] = 0.0; cap.a[2] = 1.0;
  cap.b[0] = 0.5; cap.b[1] = 0.0; cap.b[2] = 1.0;
  cap.r = 0.1;
  const double cs = capsule_sphere(cap, sphere(0.0, 0.0, 0.0, 0.2));
  rec("cs", cs, 3);
  RC_CHECK_NEAR(cs, 1.0 - 0.1 - 0.2, 1e-12);
  Box box;
  box.lo[0] = -0.5; box.lo[1] = -0.5; box.lo[2] = 0.0;
  box.hi[0] = 0.5; box.hi[1] = 0.5; box.hi[2] = 0.4;
  const double up[3] = {-1.0, 0.0, 0.9}, up2[3] = {1.0, 0.0, 0.9};
  rec("sb", seg_box_distance(up, up2, box), 3);
  RC_CHECK_NEAR(seg_box_distance(up, up2, box), 0.5, 1e-9);
  const double side[3] = {1.5, 0.0, 0.2}, side2[3] = {2.5, 0.0, 0.2};
  RC_CHECK_NEAR(seg_box_distance(side, side2, box), 1.0, 1e-9);
  const double in[3] = {-0.2, 0.0, 0.2}, in2[3] = {0.2, 0.0, 0.2};
  RC_CHECK_NEAR(seg_box_distance(in, in2, box), 0.0, 1e-9);
}

RC_TEST("the arm is above or through a floor by the height of its lowest capsule less its radius") {
  const ArmTable t = elbow_arm();
  const std::vector<double> q = q_down_at(t, 0.5, 0.0, 0.30);
  const std::vector<Capsule> caps = arm_capsules(t, q, radii(), 0.10);
  RC_CHECK(caps.size() == 4);
  double lowest = 1e9, rad = 0.0;
  for (std::size_t i = 1; i < caps.size(); ++i) {  // not the base column, which stands on the floor
    const double z = std::fmin(caps[i].a[2], caps[i].b[2]) - caps[i].r;
    if (z < lowest) {
      lowest = z;
      rad = caps[i].r;
    }
  }
  (void)rad;
  Scene low, high;
  low.has_floor = high.has_floor = true;
  low.floor_z = 0.1;
  high.floor_z = 0.4;
  const double above = scene_distance(caps, low, false).d, through = scene_distance(caps, high, false).d;
  rec("clear_floor", above, 3);
  rec("pen_floor", through, 3);
  RC_CHECK_NEAR(above, lowest - 0.1, 1e-12);
  RC_CHECK_NEAR(through, lowest - 0.4, 1e-12);
  RC_CHECK(above > 0.0);
  RC_CHECK(through < 0.0);
  RC_CHECK(collides(caps, high, false, 0.0));
  RC_CHECK(!collides(caps, low, false, 0.0));
  RC_CHECK(collides(caps, low, false, above + 0.01));
}

RC_TEST("links that share a joint are not asked, and the flange brought down beside the base touches the base column") {
  const ArmTable t = elbow_arm();
  const std::vector<Capsule> far = arm_capsules(t, q_down_at(t, 0.6, 0.0, 0.30), radii(), 0.10);
  const std::vector<Capsule> near = arm_capsules(t, q_down_at(t, 0.0, 0.0, 0.15), radii(), 0.10);
  Scene empty;
  const Clearance cf = scene_distance(far, empty, true), cn = scene_distance(near, empty, true);
  rec("pairs", 4 * 3 / 2, 0);
  rec("n_pairs", self_pairs(far.size()), 0);
  rec("self_d", cn.d, 3);
  RC_CHECK(self_pairs(4) == 3);
  RC_CHECK(cf.d > 0.0);
  RC_CHECK(cn.d < 0.0);
  RC_CHECK(cn.other == -2);
  // Without the self check there is nothing in an empty scene to hit.
  RC_CHECK(scene_distance(near, empty, false).d > 1e8);
}

RC_TEST("a sphere in the way is found, and its index and the link are reported") {
  const ArmTable t = elbow_arm();
  const std::vector<double> q = q_down_at(t, 0.5, 0.0, 0.30);
  const std::vector<Capsule> caps = arm_capsules(t, q, radii(), 0.10);
  Scene scene;
  scene.spheres.push_back(sphere(2.0, 2.0, 2.0, 0.1));
  scene.spheres.push_back(sphere(0.5, 0.0, 0.2, 0.05));  // under the tool
  const Clearance c = scene_distance(caps, scene, false);
  RC_CHECK(c.other == 1);
  RC_CHECK(c.capsule == static_cast<int>(caps.size()) - 1);
  RC_CHECK(c.d < 0.2);
}
