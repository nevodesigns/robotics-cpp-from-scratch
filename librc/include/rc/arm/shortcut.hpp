// rc/arm/shortcut.hpp
//
// Making the path shorter and smoother, from lesson 30-05.
//
// turning is the angle between two neighbouring segments of a path (zero for straight on), and max_turning the largest of them: a measure of how many sharp corners a path has. point_at gives the configuration a given distance along a path. shortcut repeats a number of times: it chooses two random distances along the path, finds the points there, and if the straight edge between them is free (by conservative advancement, with the margin given) it replaces the part of the path between them by that edge; it never makes the path longer. cut_corners replaces each interior vertex by two points a fraction of the way back along its segment and forward along the next, if the straight edge between those two is free, which rounds the corner and shortens it a little; the two outer pieces lie on the old segments and need no check. min_clearance is the smallest signed distance of the arm to the scene over the poses along a path.
//
// A path from RRT-Connect was 4.83 times the straight line in joint space with 17 points and a sharpest turn of 2.25 rad. After 150 shortcuts it was 1.85 times with 17 points (a shortcut puts in two points of its own), and after cutting the corners twice the sharpest turn was 0.60 rad. Shortcutting with no margin took the path to a clearance of 0.0001 m against the planning margin of 0.02 m; with the margin kept it was 0.0202 m. Every edge of every path was free.

#ifndef RC_RC_ARM_SHORTCUT_HPP
#define RC_RC_ARM_SHORTCUT_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/arm/rrt.hpp>
#include <vector>

namespace rc {
namespace arm {

// The angle between two neighbouring segments of a path: zero for straight on, pi for turning right back.
inline double turning(const std::vector<double>& a, const std::vector<double>& b, const std::vector<double>& c) {
  double dot = 0.0, na = 0.0, nb = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    const double u = b[i] - a[i], v = c[i] - b[i];
    dot += u * v;
    na += u * u;
    nb += v * v;
  }
  if (na < 1e-18 || nb < 1e-18) return 0.0;
  return std::acos(std::fmax(-1.0, std::fmin(1.0, dot / std::sqrt(na * nb))));
}

inline double max_turning(const std::vector<std::vector<double> >& path) {
  double worst = 0.0;
  for (std::size_t i = 1; i + 1 < path.size(); ++i) worst = std::fmax(worst, turning(path[i - 1], path[i], path[i + 1]));
  return worst;
}

// The configuration a distance s along the path, and which segment it is on.
inline std::vector<double> point_at(const std::vector<std::vector<double> >& path, double s, std::size_t& segment) {
  double run = 0.0;
  for (std::size_t i = 1; i < path.size(); ++i) {
    const double len = joint_distance(path[i - 1], path[i]);
    if (run + len >= s || i + 1 == path.size()) {
      segment = i - 1;
      const double u = len > 1e-12 ? std::fmax(0.0, std::fmin(1.0, (s - run) / len)) : 0.0;
      std::vector<double> q(path[i].size());
      for (std::size_t k = 0; k < q.size(); ++k) q[k] = path[i - 1][k] + (path[i][k] - path[i - 1][k]) * u;
      return q;
    }
    run += len;
  }
  segment = 0;
  return path.front();
}

// Chooses two random points of the path and replaces what is between them by a straight edge if that edge is free. Never lengthens the path.
inline std::vector<std::vector<double> > shortcut(const PlanProblem& p, std::vector<std::vector<double> > path, int iterations, unsigned long long seed, double margin) {
  PlanRng rng(seed);
  for (int it = 0; it < iterations && path.size() > 2; ++it) {
    const double total = path_length(path);
    double s1 = total * rng.uniform(), s2 = total * rng.uniform();
    if (s1 > s2) std::swap(s1, s2);
    if (s2 - s1 < 1e-6) continue;
    std::size_t i1 = 0, i2 = 0;
    const std::vector<double> q1 = point_at(path, s1, i1), q2 = point_at(path, s2, i2);
    if (i1 == i2) continue;
    if (!edge_conservative(p.check, q1, q2, p.bounds, margin).free) continue;
    std::vector<std::vector<double> > out(path.begin(), path.begin() + static_cast<long>(i1) + 1);
    out.push_back(q1);
    out.push_back(q2);
    out.insert(out.end(), path.begin() + static_cast<long>(i2) + 1, path.end());
    if (path_length(out) < path_length(path)) path = out;
  }
  return path;
}

// Replaces each interior vertex by two points a fraction f of the way back along its segment and forward along the next, if the edge between them is free.
inline std::vector<std::vector<double> > cut_corners(const PlanProblem& p, const std::vector<std::vector<double> >& path, double f, int rounds, double margin) {
  std::vector<std::vector<double> > cur = path;
  for (int r = 0; r < rounds; ++r) {
    std::vector<std::vector<double> > next;
    next.push_back(cur.front());
    for (std::size_t i = 1; i + 1 < cur.size(); ++i) {
      std::vector<double> a(cur[i].size()), b(cur[i].size());
      for (std::size_t k = 0; k < a.size(); ++k) {
        a[k] = cur[i][k] + f * (cur[i - 1][k] - cur[i][k]);
        b[k] = cur[i][k] + f * (cur[i + 1][k] - cur[i][k]);
      }
      if (edge_conservative(p.check, a, b, p.bounds, margin).free) {
        next.push_back(a);
        next.push_back(b);
      } else {
        next.push_back(cur[i]);
      }
    }
    next.push_back(cur.back());
    cur = next;
  }
  return cur;
}

// The smallest signed distance of the arm to the scene over the poses along a path (looked at every 0.02 rad).
inline double min_clearance(const PlanProblem& p, const std::vector<std::vector<double> >& path) {
  double worst = 1e9;
  for (std::size_t i = 1; i < path.size(); ++i) {
    const double len = joint_distance(path[i - 1], path[i]);
    const int n = static_cast<int>(std::ceil(len / 0.02));
    for (int k = 0; k <= n; ++k) {
      std::vector<double> q(path[i].size());
      for (std::size_t j = 0; j < q.size(); ++j) q[j] = path[i - 1][j] + (path[i][j] - path[i - 1][j]) * (n ? static_cast<double>(k) / n : 0.0);
      worst = std::fmin(worst, clearance_at(p.check, q));
    }
  }
  return worst;
}


}  // namespace arm
}  // namespace rc

#endif
