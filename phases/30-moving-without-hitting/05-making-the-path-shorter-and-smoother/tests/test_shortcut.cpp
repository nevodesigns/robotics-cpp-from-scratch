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
    {"len0", 4.8279442740891145},
    {"pts0", 17.0},
    {"iters", 150.0},
    {"len1", 1.8473854618876306},
    {"pts1", 17.0},
    {"turn0", 2.24538561120628},
    {"turn2", 0.59854193662405},
    {"clr0", 7.169557450410063e-05},
    {"clr1", 0.02015078561408503},
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

std::vector<double> down_q(const ArmTable& t, double x, double y, double z) {
  Pose p;
  p.r[1][1] = -1.0;
  p.r[2][2] = -1.0;
  p.p[0] = x;
  p.p[1] = y;
  p.p[2] = z;
  const std::vector<std::vector<double> > sols = ik_spherical(t, p, Pose(), trans(0.0, 0.0, 0.10));
  if (sols.empty()) return std::vector<double>(6, 99.0);
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

struct Rig {
  ArmTable table;
  std::vector<double> r;
  Scene scene;
  PlanProblem p;
  std::vector<double> start, goal;
  Rig() : table(elbow_arm()), r(radii()) {
    Box b;
    b.lo[0] = 0.32; b.lo[1] = -0.30; b.lo[2] = 0.0;
    b.hi[0] = 0.42; b.hi[1] = 0.30; b.hi[2] = 0.35;
    scene.boxes.push_back(b);
    for (std::size_t i = 0; i < table.rows.size(); ++i) {
      table.rows[i].lo = -3.0;
      table.rows[i].hi = 3.0;
    }
    p.check.table = &table;
    p.check.radii = &r;
    p.check.tool_length = 0.10;
    p.check.scene = &scene;
    p.check.self = true;
    p.bounds = sweep_bounds(table, r, 0.10);
    start = down_q(table, 0.20, 0.20, 0.30);
    goal = down_q(table, 0.55, -0.05, 0.30);
  }
};

bool free_path(const Rig& g, const std::vector<std::vector<double> >& path, double margin) {
  for (std::size_t i = 1; i < path.size(); ++i)
    if (!edge_conservative(g.p.check, path[i - 1], path[i], g.p.bounds, margin).free) return false;
  return true;
}

}  // namespace

RC_TEST("shortcutting makes a random path far shorter, keeps its ends, and never leaves a free path") {
  Rig g;
  g.p.seed = 3;
  const PlanResult plan = rrt_connect(g.p, g.start, g.goal);
  RC_CHECK(plan.found);
  const double straight = joint_distance(g.start, g.goal);
  const double l0 = path_length(plan.path);
  const int iters = 150;
  const std::vector<std::vector<double> > s = shortcut(g.p, plan.path, iters, 5, 0.02);
  rec("len0", l0 / straight, 2);
  rec("pts0", static_cast<double>(plan.path.size()), 0);
  rec("iters", iters, 0);
  rec("len1", path_length(s) / straight, 2);
  rec("pts1", static_cast<double>(s.size()), 0);
  RC_CHECK(path_length(s) < 0.7 * l0);
  RC_CHECK(joint_distance(s.front(), g.start) < 1e-12 && joint_distance(s.back(), g.goal) < 1e-12);
  RC_CHECK(free_path(g, s, 0.02));
  const std::vector<std::vector<double> > none = shortcut(g.p, plan.path, 0, 5, 0.02);
  RC_CHECK(none.size() == plan.path.size());
  double prev = l0;
  std::vector<std::vector<double> > cur = plan.path;
  for (int k = 0; k < 6; ++k) {
    cur = shortcut(g.p, cur, 10, static_cast<unsigned long long>(k + 1), 0.02);
    RC_CHECK(path_length(cur) <= prev + 1e-12);
    prev = path_length(cur);
  }
}

RC_TEST("cutting the corners rounds the sharp turns and the path is still free") {
  Rig g;
  g.p.seed = 3;
  const PlanResult plan = rrt_connect(g.p, g.start, g.goal);
  RC_CHECK(plan.found);
  const std::vector<std::vector<double> > s = shortcut(g.p, plan.path, 150, 5, 0.02);
  const std::vector<std::vector<double> > c = cut_corners(g.p, s, 0.25, 2, 0.02);
  rec("turn0", max_turning(plan.path), 2);
  rec("turn2", max_turning(c), 2);
  RC_CHECK(max_turning(c) < max_turning(s));
  RC_CHECK(path_length(c) < path_length(s) - 1e-4);
  for (std::size_t i = 1; i < c.size(); ++i) RC_CHECK(joint_distance(c[i - 1], c[i]) > 1e-9);  // no point repeated: a turn that has been hidden by a zero length segment is not a smooth one
  RC_CHECK(joint_distance(c.front(), g.start) < 1e-12 && joint_distance(c.back(), g.goal) < 1e-12);
  RC_CHECK(free_path(g, c, 0.02));
  // Where there is no room to cut a corner the vertex stays.
  std::vector<std::vector<double> > tiny;
  tiny.push_back(g.start);
  tiny.push_back(g.goal);
  RC_CHECK(cut_corners(g.p, tiny, 0.25, 1, 0.02).size() == 2);
}

RC_TEST("shortcutting with no margin quietly spends the clearance the planner kept") {
  Rig g;
  g.p.seed = 3;
  const PlanResult plan = rrt_connect(g.p, g.start, g.goal);
  RC_CHECK(plan.found);
  const std::vector<std::vector<double> > bare = shortcut(g.p, plan.path, 400, 5, 0.0);
  const std::vector<std::vector<double> > kept = shortcut(g.p, plan.path, 400, 5, 0.02);
  const double c0 = min_clearance(g.p, bare), c1 = min_clearance(g.p, kept);
  rec("clr0", c0, 4);
  rec("clr1", c1, 4);
  RC_CHECK(c1 >= 0.02 - 1e-3);
  RC_CHECK(c0 < c1);
  RC_CHECK(c0 >= 0.0);
}

RC_TEST("a point along a path is where the length says it is") {
  std::vector<std::vector<double> > path(3, std::vector<double>(6, 0.0));
  path[1][0] = 1.0;
  path[2][0] = 1.0;
  path[2][1] = 2.0;
  std::size_t seg = 0;
  const std::vector<double> a = point_at(path, 0.5, seg);
  RC_CHECK_NEAR(a[0], 0.5, 1e-12);
  RC_CHECK(seg == 0);
  const std::vector<double> b = point_at(path, 2.0, seg);
  RC_CHECK_NEAR(b[0], 1.0, 1e-12);
  RC_CHECK_NEAR(b[1], 1.0, 1e-12);
  RC_CHECK(seg == 1);
  RC_CHECK_NEAR(max_turning(path), 3.14159265358979323846 / 2.0, 1e-12);
}
