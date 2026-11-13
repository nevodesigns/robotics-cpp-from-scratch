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

const double kPi = 3.14159265358979323846;

CarPose pt(double x, double y) {
  CarPose p;
  p.x = x;
  p.y = y;
  return p;
}

// A straight of 4 m, a quarter circle to the left of radius 1.0 m, a straight of 4 m north, at 0.05 m.
std::vector<CarPose> bend() {
  std::vector<CarPose> v;
  for (double x = 0.0; x < 4.0; x += 0.05) v.push_back(pt(x, 0.0));
  for (double a = 0.05; a <= kPi / 2.0 + 1e-9; a += 0.05) v.push_back(pt(4.0 + std::sin(a), 1.0 - std::cos(a)));
  for (double y = 1.05; y <= 5.0 + 1e-9; y += 0.05) v.push_back(pt(5.0, y));
  return v;
}

double total_time(const std::vector<CarPose>& p, const std::vector<double>& v) { return profile_times(p, v).back(); }

}  // namespace

RC_TEST("no point is driven faster than the sideways limit allows") {
  const std::vector<CarPose> path = bend();
  SpeedLimits lim;
  const std::vector<double> v = speed_profile(path, lim, nullptr);
  const std::vector<double> k = path_curvature(path);
  for (std::size_t i = 0; i < path.size(); ++i) RC_CHECK(v[i] * v[i] * std::fabs(k[i]) <= lim.a_lat * (1.0 + 1e-9));
  double vmin = 1e9;
  for (std::size_t i = 0; i < path.size(); ++i) vmin = std::min(vmin, v[i]);
  RC_CHECK(vmin >= 0.0);
}

RC_TEST("the brakes are applied before the bend, within the braking limit, which the capped speeds alone do not give") {
  const std::vector<CarPose> path = bend();
  SpeedLimits lim;
  lim.v_start = lim.v_max;
  lim.v_end = lim.v_max;
  const std::vector<double> k = path_curvature(path);
  std::vector<double> naive(path.size(), lim.v_max);
  for (std::size_t i = 0; i < path.size(); ++i)
    if (std::fabs(k[i]) > 1e-9) naive[i] = std::min(naive[i], std::sqrt(lim.a_lat / std::fabs(k[i])));
  const std::vector<double> v = speed_profile(path, lim, nullptr);
  rec("decel_naive", max_decel(path, naive), 1);
  rec("decel_prof", max_decel(path, v), 2);
  rec("decel_limit", lim.a_dec, 1);
  RC_CHECK(max_decel(path, naive) > 3.0 * lim.a_dec);
  RC_CHECK(max_decel(path, v) <= lim.a_dec * (1.0 + 1e-9));
}

RC_TEST("the speeding up is within the limit, from rest") {
  std::vector<CarPose> line;
  for (double x = 0.0; x <= 10.0 + 1e-9; x += 0.05) line.push_back(pt(x, 0.0));
  SpeedLimits lim;
  lim.v_end = lim.v_max;
  const std::vector<double> v = speed_profile(line, lim, nullptr);
  RC_CHECK_NEAR(v.front(), 0.0, 1e-12);
  RC_CHECK(max_accel(line, v) <= lim.a_acc * (1.0 + 1e-9));
  RC_CHECK_NEAR(v.back(), lim.v_max, 1e-9);
}

RC_TEST("a stop line is reached at zero speed, with the slowing begun in time") {
  std::vector<CarPose> line;
  for (double x = 0.0; x <= 16.0 + 1e-9; x += 0.05) line.push_back(pt(x, 0.0));
  const std::size_t stop = 160;  // x = 8 m
  std::vector<double> caps(line.size(), 1e9);
  caps[stop] = 0.0;
  SpeedLimits lim;
  lim.v_end = lim.v_max;
  const std::vector<double> v = speed_profile(line, lim, &caps);
  RC_CHECK_NEAR(v[stop], 0.0, 1e-12);
  RC_CHECK(max_decel(line, v) <= lim.a_dec * (1.0 + 1e-9));
  // Where does the slowing begin: the last point before the stop at which the speed was the top speed.
  std::size_t first = stop;
  while (first > 0 && v[first - 1] < lim.v_max - 1e-9) --first;
  const double dist = (stop - first) * 0.05;
  rec("stop_dist", dist, 2);
  RC_CHECK(dist >= lim.v_max * lim.v_max / (2.0 * lim.a_dec) - 0.1);
}

RC_TEST("the same path is slower on ice than on tarmac") {
  const std::vector<CarPose> path = bend();
  SpeedLimits tarmac, ice;
  tarmac.a_lat = lateral_limit(0.80, 0.7);
  ice.a_lat = lateral_limit(0.10, 0.7);
  tarmac.v_end = ice.v_end = 0.0;
  tarmac.v_max = ice.v_max = 3.0;
  const double tt = total_time(path, speed_profile(path, tarmac, nullptr));
  const double ti = total_time(path, speed_profile(path, ice, nullptr));
  rec("t_tarmac", tt, 1);
  rec("t_ice", ti, 1);
  RC_CHECK(ti > tt);
}

RC_TEST("a corner of 90 degrees costs time, and smoothing it gives some back") {
  std::vector<CarPose> raw;
  raw.push_back(pt(0.0, 0.0));
  raw.push_back(pt(6.0, 0.0));
  raw.push_back(pt(6.0, 6.0));
  const std::vector<CarPose> path = resample(raw, 0.2);
  const std::vector<CarPose> smooth = smooth_path(path, nullptr, SmoothConfig()).path;
  SpeedLimits lim;
  const double tr = total_time(path, speed_profile(path, lim, nullptr));
  const double ts = total_time(smooth, speed_profile(smooth, lim, nullptr));
  rec("t_corner_raw", tr, 1);
  rec("t_corner_smooth", ts, 1);
  RC_CHECK(ts < tr);
}
