#include <rc/test/rc_test.hpp>

#include <cmath>
#include <functional>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;

rc::perceive::GridSpec field() {
  rc::perceive::GridSpec s;
  s.origin_x = 0.0;
  s.origin_y = 0.0;
  s.resolution = 0.1;
  s.width = 200;
  s.height = 200;
  return s;
}

// Inflates a grid by the car's inscribed radius so that the centre can be treated as a point.
CostGrid world(const std::vector<rc::field::Segment>& walls) {
  CostGrid g(field());
  rc::field::World2 w;
  w.walls = walls;
  rc::perceive::rasterise_world(g, w);
  rc::perceive::inflate(g, 0.2, 0.2, 3.0);
  return g;
}

CarPose pose(double x, double y, double th) {
  CarPose p;
  p.x = x;
  p.y = y;
  p.theta = th;
  return p;
}

}  // namespace

RC_TEST("on an empty field the path ends at the goal and every part of it is an arc the car can drive") {
  const CostGrid g = world({});
  HybridConfig cfg;
  const HybridResult r = hybrid_astar(g, pose(2, 2, 0.0), pose(12, 8, kPi / 2.0), cfg);
  const double limit = std::tan(cfg.max_steer) / cfg.wheelbase;
  rec("curv_limit", limit, 2);
  rec("curv_hybrid", max_curvature(r.path), 2);
  RC_CHECK(r.found);
  RC_CHECK(r.path.size() > 5);
  const CarPose last = r.path.back();
  RC_CHECK(std::hypot(last.x - 12.0, last.y - 8.0) < cfg.goal_xy);
  RC_CHECK(std::fabs(wrap_pi(last.theta - kPi / 2.0)) < cfg.goal_theta);
  RC_CHECK(max_curvature(r.path) <= limit * 1.05);
  RC_CHECK_NEAR(r.path.front().x, 2.0, 1e-12);
}

RC_TEST("the path of a grid has corners, and a corner is a curvature no car has") {
  // A path of the cells of a grid between the same two poses: east to x = 12, then north to y = 8, with a corner of 90 degrees.
  std::vector<CarPose> grid_path;
  for (double x = 2.0; x <= 12.0; x += 0.1) grid_path.push_back(pose(x, 2.0, 0.0));
  for (double y = 2.1; y <= 8.0; y += 0.1) grid_path.push_back(pose(12.0, y, kPi / 2.0));
  rec("curv_grid", max_curvature(grid_path), 1);
  RC_CHECK(max_curvature(grid_path) > 10.0);
  RC_CHECK(max_curvature(grid_path) > 5.0 * std::tan(0.4189) / 0.33);
}

RC_TEST("a wall with a gap is driven through the gap") {
  std::vector<rc::field::Segment> walls;
  walls.push_back(rc::field::Segment{8.0, 0.0, 8.0, 9.0});
  walls.push_back(rc::field::Segment{8.0, 11.0, 8.0, 20.0});
  const CostGrid g = world(walls);
  HybridConfig cfg;
  const HybridResult r = hybrid_astar(g, pose(2, 2, 0.0), pose(14, 16, 0.0), cfg);
  rec("exp_gap", r.expansions, 0);
  RC_CHECK(r.found);
  bool through = false;
  for (std::size_t i = 1; i < r.path.size(); ++i)
    if (r.path[i - 1].x < 8.0 && r.path[i].x >= 8.0) through = r.path[i].y > 9.0 && r.path[i].y < 11.0;
  RC_CHECK(through);
  RC_CHECK(max_curvature(r.path) <= std::tan(cfg.max_steer) / cfg.wheelbase * 1.05);
}

RC_TEST("finer headings in a cell cost expansions") {
  const CostGrid g = world({});
  HybridConfig a, b;
  a.theta_bins = 12;
  b.theta_bins = 48;
  const HybridResult ra = hybrid_astar(g, pose(2, 2, 0.0), pose(12, 8, kPi / 2.0), a);
  const HybridResult rb = hybrid_astar(g, pose(2, 2, 0.0), pose(12, 8, kPi / 2.0), b);
  rec("exp_12", ra.expansions, 0);
  rec("exp_48", rb.expansions, 0);
  RC_CHECK(ra.found && rb.found);
  RC_CHECK(rb.expansions >= ra.expansions);
}

RC_TEST("a goal that is walled in is reported as not found") {
  std::vector<rc::field::Segment> walls;
  walls.push_back(rc::field::Segment{11.0, 7.0, 13.0, 7.0});
  walls.push_back(rc::field::Segment{13.0, 7.0, 13.0, 9.0});
  walls.push_back(rc::field::Segment{13.0, 9.0, 11.0, 9.0});
  walls.push_back(rc::field::Segment{11.0, 9.0, 11.0, 7.0});
  const CostGrid g = world(walls);
  HybridConfig cfg;
  cfg.max_expansions = 4000;
  const HybridResult r = hybrid_astar(g, pose(2, 2, 0.0), pose(12, 8, 0.0), cfg);
  rec("exp_walled", r.expansions, 0);
  RC_CHECK(!r.found);
  RC_CHECK(r.path.empty());
  RC_CHECK(r.expansions > 0);
}
