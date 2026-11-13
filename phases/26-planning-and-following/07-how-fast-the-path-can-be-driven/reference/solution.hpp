#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/drive/smooth.hpp>
#include <vector>

using namespace rc::drive;

struct SpeedLimits {
  double v_max = 2.0;    // m/s
  double a_lat = 2.0;    // m/s2, the sideways acceleration allowed in a turn
  double a_acc = 1.0;    // m/s2, the most it speeds up
  double a_dec = 1.5;    // m/s2, the most it slows down
  double v_start = 0.0;  // m/s
  double v_end = 0.0;    // m/s, zero for a stop at the end
};

// The sideways acceleration a surface allows, with a margin of its grip (0 to 1), m/s2.
inline double lateral_limit(double mu, double margin) { return margin * mu * 9.81; }

// The curvature at each point from the points either side (the ends take their neighbour's).
inline std::vector<double> path_curvature(const std::vector<CarPose>& path) {
  std::vector<double> k(path.size(), 0.0);
  for (std::size_t i = 1; i + 1 < path.size(); ++i) k[i] = menger_curvature(path[i - 1], path[i], path[i + 1]);
  if (path.size() > 2) {
    k.front() = k[1];
    k.back() = k[path.size() - 2];
  }
  return k;
}

// A speed for each point of the path, within the limits. `caps`, if given, is a ceiling for each point (zero is a stop).
inline std::vector<double> speed_profile(const std::vector<CarPose>& path, const SpeedLimits& lim, const std::vector<double>* caps) {
  const std::size_t n = path.size();
  std::vector<double> v(n, lim.v_max);
  if (n == 0) return v;
  const std::vector<double> k = path_curvature(path);
  std::vector<double> ds(n, 0.0);
  for (std::size_t i = 0; i + 1 < n; ++i) ds[i] = std::hypot(path[i + 1].x - path[i].x, path[i + 1].y - path[i].y);
  for (std::size_t i = 0; i < n; ++i) {
    if (std::fabs(k[i]) > 1e-9) v[i] = std::min(v[i], std::sqrt(lim.a_lat / std::fabs(k[i])));
    if (caps != nullptr && i < caps->size()) v[i] = std::min(v[i], (*caps)[i]);
  }
  v[0] = std::min(v[0], lim.v_start);
  v[n - 1] = std::min(v[n - 1], lim.v_end);
  for (std::size_t i = n - 1; i-- > 0;) v[i] = std::min(v[i], std::sqrt(v[i + 1] * v[i + 1] + 2.0 * lim.a_dec * ds[i]));
  for (std::size_t i = 0; i + 1 < n; ++i) v[i + 1] = std::min(v[i + 1], std::sqrt(v[i] * v[i] + 2.0 * lim.a_acc * ds[i]));
  return v;
}

// The time at each point, from the speeds.
inline std::vector<double> profile_times(const std::vector<CarPose>& path, const std::vector<double>& v) {
  std::vector<double> t(path.size(), 0.0);
  for (std::size_t i = 0; i + 1 < path.size(); ++i) {
    const double ds = std::hypot(path[i + 1].x - path[i].x, path[i + 1].y - path[i].y);
    t[i + 1] = t[i] + 2.0 * ds / std::max(v[i] + v[i + 1], 1e-3);
  }
  return t;
}

// The hardest braking between neighbouring points, m/s2 (zero if it never slows down).
inline double max_decel(const std::vector<CarPose>& path, const std::vector<double>& v) {
  double worst = 0.0;
  for (std::size_t i = 0; i + 1 < path.size(); ++i) {
    const double ds = std::hypot(path[i + 1].x - path[i].x, path[i + 1].y - path[i].y);
    if (ds > 1e-9) worst = std::max(worst, (v[i] * v[i] - v[i + 1] * v[i + 1]) / (2.0 * ds));
  }
  return worst;
}

// The hardest speeding up between neighbouring points, m/s2.
inline double max_accel(const std::vector<CarPose>& path, const std::vector<double>& v) {
  double worst = 0.0;
  for (std::size_t i = 0; i + 1 < path.size(); ++i) {
    const double ds = std::hypot(path[i + 1].x - path[i].x, path[i + 1].y - path[i].y);
    if (ds > 1e-9) worst = std::max(worst, (v[i + 1] * v[i + 1] - v[i] * v[i]) / (2.0 * ds));
  }
  return worst;
}


#endif
