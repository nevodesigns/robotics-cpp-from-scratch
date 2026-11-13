#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/ik_closed.hpp>
#include <rc/arm/singular.hpp>
#include <vector>

using namespace rc::arm;

struct VoxelGrid {
  double res;
  double half;
  int n;
  std::vector<char> v;
  VoxelGrid(double r, double h) : res(r), half(h), n(static_cast<int>(2.0 * h / r)), v(static_cast<std::size_t>(n) * static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0) {}
  bool mark(double x, double y, double z) {
    const int i = static_cast<int>(std::floor((x + half) / res)), j = static_cast<int>(std::floor((y + half) / res)), k = static_cast<int>(std::floor((z + half) / res));
    if (i < 0 || j < 0 || k < 0 || i >= n || j >= n || k >= n) return false;
    v[(static_cast<std::size_t>(k) * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)) * static_cast<std::size_t>(n) + static_cast<std::size_t>(i)] = 1;
    return true;
  }
  bool at(int i, int j, int k) const { return v[(static_cast<std::size_t>(k) * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)) * static_cast<std::size_t>(n) + static_cast<std::size_t>(i)] != 0; }
  double cx(int i) const { return -half + (i + 0.5) * res; }
};

// Marks the cell of the tool for random joint angles inside the limits. With a floor above zero only poses whose smallest singular value is at least that are marked.
inline void sample_reach(const ArmTable& t, const Pose& base, const Pose& tool, long samples, unsigned long long seed, double sigma_floor, VoxelGrid& g) {
  unsigned long long s = seed * 2862933555777941757ULL + 3037000493ULL;
  std::vector<double> q(t.rows.size());
  for (long n = 0; n < samples; ++n) {
    for (std::size_t i = 0; i < q.size(); ++i) {
      s = s * 6364136223846793005ULL + 1442695040888963407ULL;
      const double u = static_cast<double>((s >> 33) & 0xFFFFFFULL) / 16777216.0;
      q[i] = t.rows[i].lo + (t.rows[i].hi - t.rows[i].lo) * u;
    }
    if (sigma_floor > 0.0 && smallest_singular(jacobian(t, q, base, tool, t.convention)) < sigma_floor) continue;
    const Pose p = forward(t, q, base, tool);
    g.mark(p.p[0], p.p[1], p.p[2]);
  }
}

inline double volume(const VoxelGrid& g) {
  long count = 0;
  for (std::size_t i = 0; i < g.v.size(); ++i) count += g.v[i] ? 1 : 0;
  return static_cast<double>(count) * g.res * g.res * g.res;
}

// The greatest distance of a marked cell from the base.
inline double farthest(const VoxelGrid& g) {
  double best = 0.0;
  for (int k = 0; k < g.n; ++k)
    for (int j = 0; j < g.n; ++j)
      for (int i = 0; i < g.n; ++i)
        if (g.at(i, j, k)) best = std::fmax(best, std::sqrt(g.cx(i) * g.cx(i) + g.cx(j) * g.cx(j) + g.cx(k) * g.cx(k)));
  return best;
}

// The greatest horizontal distance from the base axis of a marked cell with height between z0 and z1.
inline double farthest_horizontal(const VoxelGrid& g, double z0, double z1) {
  double best = 0.0;
  for (int k = 0; k < g.n; ++k) {
    if (g.cx(k) < z0 || g.cx(k) > z1) continue;
    for (int j = 0; j < g.n; ++j)
      for (int i = 0; i < g.n; ++i)
        if (g.at(i, j, k)) best = std::fmax(best, std::hypot(g.cx(i), g.cx(j)));
  }
  return best;
}

// Can the arm (a spherical wrist) put its tool at a point, pointing straight down, inside its limits?
inline bool tool_down_reachable(const ArmTable& t, double x, double y, double z, const Pose& tool) {
  Pose target;
  target.r[1][1] = -1.0;
  target.r[2][2] = -1.0;
  target.p[0] = x;
  target.p[1] = y;
  target.p[2] = z;
  (void)t;  // TODO: ask ik_spherical for the solutions with the tool pointing down (the rotation with the y and z axes reversed) and keep those inside the limits
  (void)x;
  (void)y;
  (void)z;
  (void)tool;
  return false;
}

// The farthest point along the x axis, at a height, that the tool can reach pointing down (stepping out by 1 cm).
inline double tool_down_radius(const ArmTable& t, double z, const Pose& tool) {
  double best = 0.0;
  for (double x = 0.0; x < 3.0; x += 0.01)
    if (tool_down_reachable(t, x, 0.0, z, tool)) best = x;
  return best;
}

// The share of a rectangle of points, a step apart, at a height, that the tool can reach pointing down.
inline double table_coverage(const ArmTable& t, const Pose& tool, double x0, double x1, double y0, double y1, double z, double step) {
  long total = 0, ok = 0;
  for (double x = x0; x <= x1 + 1e-9; x += step)
    for (double y = y0; y <= y1 + 1e-9; y += step) {
      ++total;
      if (tool_down_reachable(t, x, y, z, tool)) ++ok;
    }
  return total ? static_cast<double>(ok) / static_cast<double>(total) : 0.0;
}


#endif
