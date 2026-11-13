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

Scene wall_scene() {
  Scene s;
  Box b;
  b.lo[0] = 0.32; b.lo[1] = -0.30; b.lo[2] = 0.0;
  b.hi[0] = 0.42; b.hi[1] = 0.30; b.hi[2] = 0.35;
  s.boxes.push_back(b);
  return s;
}

struct Rig {
  ArmTable table;
  std::vector<double> r;
  Scene scene;
  PlanProblem p;
  std::vector<double> start, goal;
  Rig(const Scene& sc) : table(elbow_arm()), r(radii()), scene(sc) {
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

bool path_is_free(const Rig& g, const std::vector<std::vector<double> >& path) {
  for (std::size_t i = 1; i < path.size(); ++i)
    if (!edge_conservative(g.p.check, path[i - 1], path[i], g.p.bounds, 0.0).free) return false;
  return true;
}

}  // namespace

RC_TEST("in free space a path is found at once and goes from the start to the goal") {
  Scene empty;
  Rig g(empty);
  const PlanResult a = rrt(g.p, g.start, g.goal), b = rrt_connect(g.p, g.start, g.goal);
  RC_CHECK(a.found && b.found);
  RC_CHECK(joint_distance(a.path.front(), g.start) < 1e-12 && joint_distance(a.path.back(), g.goal) < 1e-12);
  RC_CHECK(joint_distance(b.path.front(), g.start) < 1e-12 && joint_distance(b.path.back(), g.goal) < 1e-12);
  RC_CHECK(path_is_free(g, a.path) && path_is_free(g, b.path));
}

RC_TEST("with a wall between, two trees find a path faster than one, and every edge of every path is free") {
  const Scene wall = wall_scene();
  const int seeds = 6;
  int found_c = 0, found_r = 0;
  double nodes_c = 0.0, nodes_r = 0.0, jag = 0.0;
  int jag_n = 0;
  for (int k = 1; k <= seeds; ++k) {
    Rig g(wall);
    g.p.seed = static_cast<unsigned long long>(k);
    const PlanResult b = rrt_connect(g.p, g.start, g.goal);
    const PlanResult a = rrt(g.p, g.start, g.goal);
    if (b.found) {
      ++found_c;
      nodes_c += b.nodes;
      RC_CHECK(path_is_free(g, b.path));
      jag += path_length(b.path) / joint_distance(g.start, g.goal);
      ++jag_n;
    }
    if (a.found) {
      ++found_r;
      nodes_r += a.nodes;
      RC_CHECK(path_is_free(g, a.path));
    }
  }
  rec("seeds", seeds, 0);
  rec("found_c", found_c, 0);
  rec("found_r", found_r, 0);
  rec("nodes_c", found_c ? nodes_c / found_c : 0.0, 0);
  rec("nodes_r", found_r ? nodes_r / found_r : 0.0, 0);
  rec("jag", jag_n ? jag / jag_n : 0.0, 2);
  RC_CHECK(found_c >= seeds - 1);
  RC_CHECK(found_r >= 1);
  RC_CHECK(nodes_c / found_c < nodes_r / found_r);
  RC_CHECK(jag / jag_n > 1.2);
}

RC_TEST("a goal inside the wall is never found and the planner stops at its limit; a start in collision stops at once") {
  const Scene wall = wall_scene();
  Rig g(wall);
  g.p.max_iter = 600;
  const std::vector<double> bad_goal = down_q(g.table, 0.37, 0.0, 0.30);  // inside the wall
  const PlanResult a = rrt_connect(g.p, g.start, bad_goal);
  rec("goal_iters", a.iterations, 0);
  RC_CHECK(!a.found);
  RC_CHECK(a.iterations == 0);
  const PlanResult b = rrt_connect(g.p, bad_goal, g.goal);
  rec("start_iters", b.iterations, 0);
  RC_CHECK(!b.found && b.iterations == 0);
  // A goal that is free but cannot be reached in the iterations given: a tiny step and a few iterations.
  g.p.max_iter = 5;
  g.p.eta = 0.05;
  const PlanResult c = rrt_connect(g.p, g.start, g.goal);
  rec("cap", c.iterations, 0);
  RC_CHECK(!c.found);
  RC_CHECK(c.iterations == 5);
}

RC_TEST("the same seed gives the same path and another seed another") {
  const Scene wall = wall_scene();
  Rig g1(wall), g2(wall), g3(wall);
  g1.p.seed = g2.p.seed = 3;
  g3.p.seed = 4;
  const PlanResult a = rrt_connect(g1.p, g1.start, g1.goal), b = rrt_connect(g2.p, g2.start, g2.goal), c = rrt_connect(g3.p, g3.start, g3.goal);
  RC_CHECK(a.found && b.found && c.found);
  RC_CHECK(a.path.size() == b.path.size());
  for (std::size_t i = 0; i < a.path.size(); ++i) RC_CHECK(joint_distance(a.path[i], b.path[i]) < 1e-12);
  RC_CHECK(a.path.size() != c.path.size() || joint_distance(a.path[1], c.path[1]) > 1e-6);
}

RC_TEST("steering goes the step and no further, and reaches a target that is near") {
  std::vector<double> a(6, 0.0), b(6, 0.0);
  b[0] = 3.0;
  const std::vector<double> s = steer(a, b, 0.5);
  RC_CHECK_NEAR(s[0], 0.5, 1e-12);
  b[0] = 0.3;
  RC_CHECK_NEAR(steer(a, b, 0.5)[0], 0.3, 1e-12);
  RC_CHECK_NEAR(joint_distance(a, b), 0.3, 1e-12);
}
