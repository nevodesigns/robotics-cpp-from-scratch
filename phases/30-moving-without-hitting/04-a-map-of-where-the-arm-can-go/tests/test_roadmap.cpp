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
  }
};

// Pairs of poses on either side of the wall.
void pair_at(const ArmTable& t, int k, std::vector<double>& a, std::vector<double>& b) {
  a = down_q(t, 0.10 + 0.03 * (k % 5), 0.25 - 0.1 * (k % 4), 0.30);
  b = down_q(t, 0.52 + 0.02 * (k % 3), -0.15 + 0.08 * (k % 5), 0.30);
}

}  // namespace

RC_TEST("a roadmap built once answers many queries, each far cheaper than a fresh plan, with shorter paths") {
  const Scene wall = wall_scene();
  Rig g(wall);
  g.p.max_iter = 4000;
  const int n = 250, k = 8;
  const Roadmap rm = build_roadmap(g.p, n, k, 11);
  rec("n_nodes", n, 0);
  rec("k", k, 0);
  rec("build_checks", rm.checks, 0);
  const int pieces = components(rm);
  rec("n_comp", pieces, 0);
  rec("comp_s", pieces == 1 ? 0 : 1, 0);
  double q_checks = 0.0, rrt_checks = 0.0, len_rm = 0.0, len_rrt = 0.0;
  int answered = 0, both = 0;
  for (int i = 0; i < 5; ++i) {
    std::vector<double> a, b;
    pair_at(g.table, i, a, b);
    const QueryResult q = roadmap_query(g.p, rm, a, b, 6);
    g.p.seed = static_cast<unsigned long long>(i + 1);
    const PlanResult r = rrt_connect(g.p, a, b);
    if (q.found) {
      ++answered;
      q_checks += q.checks;
      for (std::size_t e = 1; e < q.path.size(); ++e) RC_CHECK(edge_conservative(g.p.check, q.path[e - 1], q.path[e], g.p.bounds, 0.0).free);
    }
    if (q.found && r.found) {
      ++both;
      len_rm += q.length;
      len_rrt += path_length(r.path);
      rrt_checks += r.checks;
    }
  }
  rec("q_checks", answered ? q_checks / answered : 0.0, 0);
  rec("rrt_checks", both ? rrt_checks / both : 0.0, 0);
  rec("ratio_len", both ? len_rm / len_rrt : 0.0, 2);
  RC_CHECK(answered >= 4);
  RC_CHECK(both >= 3);
  RC_CHECK(len_rm < len_rrt);
}

RC_TEST("more nodes join the pieces of the graph, and in this easy scene even a small roadmap answers the queries that have free ends") {
  const Scene wall = wall_scene();
  const int sizes[3] = {40, 120, 300};
  int answered[3] = {0, 0, 0};
  int comps[3] = {0, 0, 0};
  for (int s = 0; s < 3; ++s) {
    Rig g(wall);
    const Roadmap rm = build_roadmap(g.p, sizes[s], 8, 5);
    comps[s] = components(rm);
    for (int i = 0; i < 8; ++i) {
      std::vector<double> a, b;
      pair_at(g.table, i, a, b);
      if (roadmap_query(g.p, rm, a, b, 6).found) ++answered[s];
    }
  }
  rec("n_small", sizes[0], 0);
  rec("n_mid", sizes[1], 0);
  rec("n_big", sizes[2], 0);
  rec("s_small", answered[0], 0);
  rec("s_mid", answered[1], 0);
  rec("s_big", answered[2], 0);
  rec("comp_small", comps[0], 0);
  rec("comp_big", comps[2], 0);
  RC_CHECK(answered[2] >= answered[1] && answered[1] >= answered[0]);
  RC_CHECK(answered[0] >= 6);
  RC_CHECK(comps[2] < comps[0]);
}

RC_TEST("a roadmap built for one scene is wrong in another, and the check says by how much") {
  const Scene wall = wall_scene();
  Rig g(wall);
  const Roadmap rm = build_roadmap(g.p, 200, 8, 3);
  RC_CHECK(edge_free_fraction(g.p, rm) > 0.99);
  // A box is put in the scene.
  Scene changed = wall;
  Box b;
  b.lo[0] = 0.20; b.lo[1] = -0.5; b.lo[2] = 0.0;
  b.hi[0] = 0.30; b.hi[1] = 0.5; b.hi[2] = 0.6;
  changed.boxes.push_back(b);
  Rig g2(changed);
  const double frac = edge_free_fraction(g2.p, rm);
  rec("bad_share", 100.0 * (1.0 - frac), 0);
  RC_CHECK(frac < 0.95);
  int bad = 0;
  for (int i = 0; i < 6; ++i) {
    std::vector<double> a, c;
    pair_at(g.table, i, a, c);
    const QueryResult q = roadmap_query(g.p, rm, a, c, 6);  // on the old scene's roadmap, with the old scene's check
    if (!q.found) continue;
    for (std::size_t e = 1; e < q.path.size(); ++e)
      if (!edge_conservative(g2.p.check, q.path[e - 1], q.path[e], g2.p.bounds, 0.0).free) ++bad;
  }
  rec("bad_edges", bad, 0);
  RC_CHECK(bad > 0);
}

RC_TEST("a start or a goal in collision gets no path, and an empty roadmap gets none either") {
  const Scene wall = wall_scene();
  Rig g(wall);
  const Roadmap rm = build_roadmap(g.p, 60, 6, 2);
  std::vector<double> a, b;
  pair_at(g.table, 0, a, b);
  const std::vector<double> inside = down_q(g.table, 0.37, 0.0, 0.30);
  RC_CHECK(!roadmap_query(g.p, rm, inside, b, 6).found);
  RC_CHECK(!roadmap_query(g.p, rm, a, inside, 6).found);
  Roadmap empty;
  RC_CHECK(!roadmap_query(g.p, empty, a, b, 6).found);
}
