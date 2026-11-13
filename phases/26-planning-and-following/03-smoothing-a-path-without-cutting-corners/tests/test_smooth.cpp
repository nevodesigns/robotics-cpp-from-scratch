#include <rc/test/rc_test.hpp>

#include <cmath>
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

CarPose pt(double x, double y) {
  CarPose p;
  p.x = x;
  p.y = y;
  return p;
}

// East along y = 2 from x = 2 to x = 8, then north along x = 8 to y = 8: a corner of 90 degrees at (8, 2).
std::vector<CarPose> corner_path() {
  std::vector<CarPose> raw;
  raw.push_back(pt(2.0, 2.0));
  raw.push_back(pt(8.0, 2.0));
  raw.push_back(pt(8.0, 8.0));
  return resample(raw, 0.2);
}

CostGrid world_with_block() {
  rc::perceive::GridSpec s;
  s.origin_x = 0.0;
  s.origin_y = 0.0;
  s.resolution = 0.05;
  s.width = 300;
  s.height = 300;
  CostGrid g(s);
  rc::field::World2 w;
  // A small block just inside the corner.
  w.walls.push_back(rc::field::Segment{7.3, 2.3, 7.7, 2.3});
  w.walls.push_back(rc::field::Segment{7.7, 2.3, 7.7, 2.7});
  w.walls.push_back(rc::field::Segment{7.7, 2.7, 7.3, 2.7});
  w.walls.push_back(rc::field::Segment{7.3, 2.7, 7.3, 2.3});
  rc::perceive::rasterise_world(g, w);
  rc::perceive::inflate(g, 0.2, 0.2, 3.0);
  return g;
}

int blocked_points(const CostGrid& g, const std::vector<CarPose>& path) {
  int n = 0;
  for (std::size_t i = 0; i < path.size(); ++i)
    if (cell_blocked(&g, path[i].x, path[i].y)) ++n;
  return n;
}

double deviation(const std::vector<CarPose>& a, const std::vector<CarPose>& b) {
  double worst = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) worst = std::max(worst, std::hypot(a[i].x - b[i].x, a[i].y - b[i].y));
  return worst;
}

}  // namespace

RC_TEST("resampling spaces the points evenly and keeps both ends and the length") {
  const std::vector<CarPose> p = corner_path();
  RC_CHECK_NEAR(p.front().x, 2.0, 1e-12);
  RC_CHECK_NEAR(p.back().y, 8.0, 1e-12);
  double length = 0.0;
  for (std::size_t i = 1; i < p.size(); ++i) {
    const double d = std::hypot(p[i].x - p[i - 1].x, p[i].y - p[i - 1].y);
    length += d;
    if (i + 1 < p.size()) RC_CHECK_NEAR(d, 0.2, 1e-9);
  }
  RC_CHECK_NEAR(length, 12.0, 1e-6);
}

RC_TEST("smoothing lowers the sharpest curvature of a path with a corner, and the ends stay") {
  const std::vector<CarPose> p = corner_path();
  const SmoothResult s = smooth_path(p, nullptr, SmoothConfig());
  const double before = max_menger_curvature(p);
  const double after = max_menger_curvature(s.path);
  rec("curv_before", before, 1);
  rec("curv_after", after, 2);
  rec("curv_limit", std::tan(0.4189) / 0.33, 2);
  rec("iters", s.sweeps, 0);
  RC_CHECK(before > 5.0);
  RC_CHECK(after < before / 1.5);
  RC_CHECK_NEAR(s.path.front().x, p.front().x, 1e-12);
  RC_CHECK_NEAR(s.path.back().y, p.back().y, 1e-12);
  RC_CHECK(s.sweeps < 2000);
}

RC_TEST("a smoother that does not look at the walls cuts the corner into one, and one that does cannot") {
  const CostGrid g = world_with_block();
  const std::vector<CarPose> p = corner_path();
  RC_CHECK(blocked_points(g, p) == 0);
  const SmoothResult free_run = smooth_path(p, nullptr, SmoothConfig());
  const SmoothResult checked = smooth_path(p, &g, SmoothConfig());
  rec("hits_free", blocked_points(g, free_run.path), 0);
  rec("hits_checked", blocked_points(g, checked.path), 0);
  RC_CHECK(blocked_points(g, free_run.path) > 0);
  RC_CHECK(blocked_points(g, checked.path) == 0);
}

RC_TEST("a heavy smoother wanders further from the plan than a light one") {
  const std::vector<CarPose> p = corner_path();
  SmoothConfig light, heavy;
  light.data_weight = 0.5;
  light.smooth_weight = 0.1;
  heavy.data_weight = 0.02;
  heavy.smooth_weight = 0.4;
  const double dl = deviation(p, smooth_path(p, nullptr, light).path);
  const SmoothResult hr = smooth_path(p, nullptr, heavy);
  const double dh = deviation(p, hr.path);
  rec("curv_heavy", max_menger_curvature(hr.path), 2);
  rec("dev_light", dl, 2);
  rec("dev_heavy", dh, 2);
  RC_CHECK(dh > dl);
}

RC_TEST("a straight path is left where it is") {
  std::vector<CarPose> raw;
  raw.push_back(pt(1.0, 1.0));
  raw.push_back(pt(7.0, 9.0));
  const std::vector<CarPose> p = resample(raw, 0.2);
  const SmoothResult s = smooth_path(p, nullptr, SmoothConfig());
  RC_CHECK(deviation(p, s.path) < 1e-9);
  RC_CHECK(max_menger_curvature(p) < 1e-6);
}
