// rc/arm/reach3d.hpp
//
// Where the arm can work, from lesson 28-08.
//
// VoxelGrid is a cube of cells of a size, centred on the base, with a mark for each cell that the tool has been seen in. sample_reach draws joint angles uniformly inside the limits of a table, finds the tool by forward kinematics and marks the cell; with a floor on the smallest singular value it marks only the poses that are well away from a singularity (the dexterous ones). volume is the marked cells times the volume of a cell, farthest the greatest distance of a marked cell from the base, and farthest_horizontal the greatest distance from the base axis within a band of heights. tool_down_reachable asks whether the arm with a spherical wrist can put its tool at a point with the tool pointing straight down, inside its limits, by the closed form of lesson 28-04; tool_down_radius is how far out along the x axis, at a height, it can; table_coverage is the share of a rectangle of points that can be.
//
// In cells of 0.10 m, 2 thousand random poses of the typical elbow arm gave a volume of 1.081 m3, 20 thousand 2.581 and 200 thousand 2.958; in cells of 0.05 m, 60 thousand gave 2.033 and 200 thousand 2.554, still rising: the sample must be many per cell. The farthest reach of the UR5 was 1.05 m from its base, the Panda's 1.16 m. At a table height of 0.15 m the farthest the typical arm's tool could reach in any orientation was 0.86 m from the base axis, and pointing straight down 0.74 m. The dexterous poses (smallest singular value of 0.1 or more) were 62 percent of the reachable volume. The elbow arm could reach 96 percent of a table 0.15 m high, with the tool down, with no joint limits, 96 percent with limits of plus and minus 120 degrees and 78 percent with limits of plus and minus 60 degrees.

#ifndef RC_RC_ARM_REACH3D_HPP
#define RC_RC_ARM_REACH3D_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/ik_closed.hpp>
#include <rc/arm/singular.hpp>
#include <vector>

namespace rc {
namespace arm {

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
  return !within_limits_only(t, ik_spherical(t, target, Pose(), tool)).empty();
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


}  // namespace arm
}  // namespace rc

#endif
