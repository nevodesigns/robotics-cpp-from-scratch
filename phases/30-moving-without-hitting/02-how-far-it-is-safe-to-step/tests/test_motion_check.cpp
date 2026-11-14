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
    {"b_total", 1.33},
    {"n_checks", 200.0},
    {"worst_ratio", 0.2893066713973122},
    {"edge_len", 2.0},
    {"fixed_found", 0.0},
    {"cons_found", 1.0},
    {"cons_checks", 14.0},
    {"fine_checks", 70.0},
    {"far_checks", 2.0},
    {"far_fixed", 201.0},
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

std::vector<double> radii() {
  std::vector<double> r(6, 0.05);
  r[0] = 0.08;
  r[1] = 0.06;
  r[3] = 0.05;
  r[5] = 0.04;
  return r;
}

std::vector<double> lerp(const std::vector<double>& a, const std::vector<double>& b, double u) {
  std::vector<double> q(a.size());
  for (std::size_t i = 0; i < a.size(); ++i) q[i] = a[i] + (b[i] - a[i]) * u;
  return q;
}

}  // namespace

RC_TEST("no point of the arm moves further than the bound says, for moves of any size") {
  const ArmTable t = elbow_arm();
  const std::vector<double> bounds = sweep_bounds(t, radii(), 0.10);
  double total = 0.0;
  for (std::size_t i = 0; i < 6; ++i) total = std::fmax(total, bounds[i]);
  rec("b_total", total, 2);
  RC_CHECK(bounds[0] > bounds[3]);
  double worst = 0.0;
  int n = 0;
  for (int k = 0; k < 200; ++k) {
    std::vector<double> a(6), b(6);
    for (int i = 0; i < 6; ++i) {
      a[static_cast<std::size_t>(i)] = 1.5 * std::sin(1.7 * (i + 1) + 0.9 * k);
      b[static_cast<std::size_t>(i)] = 1.5 * std::sin(2.9 * (i + 1) + 1.3 * k);
    }
    std::vector<double> dq(6);
    for (int i = 0; i < 6; ++i) dq[static_cast<std::size_t>(i)] = b[static_cast<std::size_t>(i)] - a[static_cast<std::size_t>(i)];
    const std::vector<Capsule> ca = arm_capsules(t, a, radii(), 0.10), cb = arm_capsules(t, b, radii(), 0.10);
    const double bound = sweep_distance(bounds, dq);
    for (std::size_t c = 0; c < ca.size() && c < cb.size(); ++c) {
      for (int e = 0; e < 2; ++e) {
        const double* pa = e == 0 ? ca[c].a : ca[c].b;
        const double* pb = e == 0 ? cb[c].a : cb[c].b;
        const double moved = std::sqrt((pa[0] - pb[0]) * (pa[0] - pb[0]) + (pa[1] - pb[1]) * (pa[1] - pb[1]) + (pa[2] - pb[2]) * (pa[2] - pb[2]));
        worst = std::fmax(worst, moved / bound);
      }
    }
    ++n;
  }
  rec("n_checks", n, 0);
  rec("worst_ratio", worst, 3);
  RC_CHECK(worst <= 1.0);
  RC_CHECK(worst > 0.05);
}

RC_TEST("a thin obstacle between the steps is missed by fixed steps and found by conservative advancement") {
  const ArmTable t = elbow_arm();
  const std::vector<double> r = radii();
  const std::vector<double> bounds = sweep_bounds(t, r, 0.10);
  std::vector<double> a(6, 0.0), b(6, 0.0);
  a[0] = -1.0;
  a[1] = 0.6;
  a[2] = 0.3;
  b[0] = 1.0;
  b[1] = 0.9;
  b[2] = 0.5;
  double edge_len = 0.0;
  for (int i = 0; i < 6; ++i) edge_len = std::fmax(edge_len, std::fabs(b[static_cast<std::size_t>(i)] - a[static_cast<std::size_t>(i)]));
  rec("edge_len", edge_len, 1);
  // A sphere of 5 mm at the tip of the tool at a share of the edge that the coarse steps do not land on.
  const double u = 0.37;
  const std::vector<Capsule> mid = arm_capsules(t, lerp(a, b, u), r, 0.10);
  Scene scene;
  Sphere s;
  for (int k = 0; k < 3; ++k) s.c[k] = mid.back().b[k];
  s.r = 0.005;
  scene.spheres.push_back(s);
  ArmCheck c;
  c.table = &t;
  c.radii = &r;
  c.tool_length = 0.10;
  c.scene = &scene;
  c.self = false;
  const EdgeResult coarse = edge_fixed(c, a, b, 0.5, 0.0);
  const EdgeResult fine = edge_fixed(c, a, b, 0.01, 0.0);
  const EdgeResult cons = edge_conservative(c, a, b, bounds, 0.0);
  rec("fixed_found", coarse.free ? 0 : 1, 0);
  rec("cons_found", cons.free ? 0 : 1, 0);
  rec("cons_checks", cons.checks, 0);
  rec("fine_checks", fine.checks, 0);
  RC_CHECK(coarse.free);
  RC_CHECK(!fine.free);
  RC_CHECK(!cons.free);
}

RC_TEST("far from anything conservative advancement looks at far fewer poses than fixed steps") {
  const ArmTable t = elbow_arm();
  const std::vector<double> r = radii();
  const std::vector<double> bounds = sweep_bounds(t, r, 0.10);
  Scene scene;
  Sphere s;
  s.c[0] = 3.0;
  s.c[1] = 3.0;
  s.c[2] = 3.0;
  s.r = 0.1;
  scene.spheres.push_back(s);
  ArmCheck c;
  c.table = &t;
  c.radii = &r;
  c.tool_length = 0.10;
  c.scene = &scene;
  c.self = false;
  std::vector<double> a(6, 0.0), b(6, 0.0);
  a[0] = -1.0;
  a[1] = 0.6;
  b[0] = 1.0;
  b[1] = 0.9;
  const EdgeResult cons = edge_conservative(c, a, b, bounds, 0.02);
  const EdgeResult fixed = edge_fixed(c, a, b, 0.01, 0.02);
  rec("far_checks", cons.checks, 0);
  rec("far_fixed", fixed.checks, 0);
  RC_CHECK(cons.free && fixed.free);
  RC_CHECK(cons.checks * 3 < fixed.checks);
}

RC_TEST("a margin turns a clear edge into one that is not free") {
  const ArmTable t = elbow_arm();
  const std::vector<double> r = radii();
  const std::vector<double> bounds = sweep_bounds(t, r, 0.10);
  std::vector<double> a(6, 0.0), b(6, 0.0);
  a[1] = 0.6;
  b[1] = 0.6;
  const std::vector<Capsule> cap = arm_capsules(t, a, r, 0.10);
  Scene scene;
  Sphere s;
  for (int k = 0; k < 3; ++k) s.c[k] = cap.back().b[k];
  s.c[0] += cap.back().r + 0.03 + 0.05;  // 3 cm clear of the tool's surface (the sphere's own radius is 0.05)
  s.r = 0.05;
  scene.spheres.push_back(s);
  ArmCheck c;
  c.table = &t;
  c.radii = &r;
  c.tool_length = 0.10;
  c.scene = &scene;
  c.self = false;
  const double d = clearance_at(c, a);
  RC_CHECK(d > 0.0 && d < 0.05);
  RC_CHECK(free_pose(c, a, 0.0));
  RC_CHECK(!free_pose(c, a, 0.05));
  RC_CHECK(edge_conservative(c, a, b, bounds, 0.0).free);
  RC_CHECK(!edge_conservative(c, a, b, bounds, 0.05).free);
}
