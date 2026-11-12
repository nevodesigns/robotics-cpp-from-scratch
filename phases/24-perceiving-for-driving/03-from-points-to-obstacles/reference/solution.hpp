#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/field/frames.hpp>
#include <rc/perceive/scan.hpp>
#include <vector>

using namespace rc::perceive;

// An object: the indices of its points, the centre of them, and the size of the box round them.
struct Cluster {
  std::vector<std::size_t> members;
  double cx = 0.0;
  double cy = 0.0;
  double size_x = 0.0;
  double size_y = 0.0;
};

inline std::size_t cluster_root(std::vector<std::size_t>& parent, std::size_t i) {
  while (parent[i] != i) {
    parent[i] = parent[parent[i]];
    i = parent[i];
  }
  return i;
}

// Groups points (x and y; the sensor is at the origin) into objects. Two points are of one object if they are closer than base_radius + range_gain * the larger of their distances from the origin, directly or through other points. Groups of fewer than min_points are dropped.
inline std::vector<Cluster> cluster_points(const std::vector<rc::field::Point3>& pts, double base_radius, double range_gain, std::size_t min_points) {
  std::vector<std::size_t> parent(pts.size());
  for (std::size_t i = 0; i < pts.size(); ++i) parent[i] = i;
  for (std::size_t i = 0; i < pts.size(); ++i) {
    const double ri = std::hypot(pts[i].x, pts[i].y);
    for (std::size_t j = i + 1; j < pts.size(); ++j) {
      const double rj = std::hypot(pts[j].x, pts[j].y);
      const double limit = base_radius + range_gain * std::max(ri, rj);
      const double dx = pts[i].x - pts[j].x;
      const double dy = pts[i].y - pts[j].y;
      if (dx * dx + dy * dy <= limit * limit) {
        const std::size_t a = cluster_root(parent, i);
        const std::size_t b = cluster_root(parent, j);
        if (a != b) parent[b] = a;
      }
    }
  }
  std::vector<Cluster> clusters;
  std::vector<std::size_t> slot(pts.size(), pts.size());
  for (std::size_t i = 0; i < pts.size(); ++i) {
    const std::size_t root = cluster_root(parent, i);
    if (slot[root] == pts.size()) {
      slot[root] = clusters.size();
      clusters.push_back(Cluster());
    }
    clusters[slot[root]].members.push_back(i);
  }
  std::vector<Cluster> kept;
  for (std::size_t c = 0; c < clusters.size(); ++c) {
    if (clusters[c].members.size() < min_points) continue;
    Cluster k = clusters[c];
    double lo_x = 1e300, hi_x = -1e300, lo_y = 1e300, hi_y = -1e300;
    for (std::size_t m = 0; m < k.members.size(); ++m) {
      const rc::field::Point3& p = pts[k.members[m]];
      k.cx += p.x;
      k.cy += p.y;
      lo_x = std::min(lo_x, p.x);
      hi_x = std::max(hi_x, p.x);
      lo_y = std::min(lo_y, p.y);
      hi_y = std::max(hi_y, p.y);
    }
    k.cx /= static_cast<double>(k.members.size());
    k.cy /= static_cast<double>(k.members.size());
    k.size_x = hi_x - lo_x;
    k.size_y = hi_y - lo_y;
    kept.push_back(k);
  }
  return kept;
}


#endif
