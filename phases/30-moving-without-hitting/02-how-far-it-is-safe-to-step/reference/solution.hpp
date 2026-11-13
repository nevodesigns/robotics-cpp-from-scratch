#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/arm/collision.hpp>
#include <vector>

using namespace rc::arm;

// How far the farthest point of the arm can move for a radian of each joint: the lengths of the links from that joint outward, the tool and the largest radius.
inline std::vector<double> sweep_bounds(const ArmTable& t, const std::vector<double>& radii, double tool_length) {
  const std::size_t n = t.rows.size();
  std::vector<double> len(n, 0.0);
  const std::vector<Pose> f0 = link_frames(t, std::vector<double>(n, 0.0), Pose());
  for (std::size_t k = 0; k < n; ++k) {
    double d2 = 0.0;
    for (int c = 0; c < 3; ++c) {
      const double prev = k == 0 ? 0.0 : f0[k - 1].p[c];
      d2 += (f0[k].p[c] - prev) * (f0[k].p[c] - prev);
    }
    len[k] = std::sqrt(d2);
  }
  double rmax = 0.0;
  for (std::size_t i = 0; i < radii.size(); ++i) rmax = std::fmax(rmax, radii[i]);
  std::vector<double> b(n, 0.0);
  double beyond = tool_length + rmax;
  for (std::size_t i = n; i-- > 0;) {
    beyond += len[i];
    b[i] = beyond;
  }
  return b;
}

// The most that any point of the arm can move for a change of the joint angles.
inline double sweep_distance(const std::vector<double>& bounds, const std::vector<double>& dq) {
  double s = 0.0;
  for (std::size_t i = 0; i < dq.size(); ++i) s += bounds[i] * std::fabs(dq[i]);
  return s;
}

struct ArmCheck {
  const ArmTable* table;
  const std::vector<double>* radii;
  double tool_length;
  const Scene* scene;
  bool self;
};

inline double clearance_at(const ArmCheck& c, const std::vector<double>& q) {
  return scene_distance(arm_capsules(*c.table, q, *c.radii, c.tool_length), *c.scene, c.self).d;
}

inline bool free_pose(const ArmCheck& c, const std::vector<double>& q, double margin) { return clearance_at(c, q) >= margin; }

struct EdgeResult {
  bool free = true;
  int checks = 0;
};

// A straight line in joint space between two configurations, looked at in steps of the same size (the largest joint change in a step is `step` radians).
inline EdgeResult edge_fixed(const ArmCheck& c, const std::vector<double>& a, const std::vector<double>& b, double step, double margin) {
  EdgeResult out;
  double span = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) span = std::fmax(span, std::fabs(b[i] - a[i]));
  const int n = static_cast<int>(std::ceil(span / step));
  const int count = n < 1 ? 1 : n;
  for (int k = 0; k <= count; ++k) {
    std::vector<double> q(a.size());
    for (std::size_t i = 0; i < a.size(); ++i) q[i] = a[i] + (b[i] - a[i]) * k / count;
    ++out.checks;
    if (!free_pose(c, q, margin)) {
      out.free = false;
      return out;
    }
  }
  return out;
}

// The same, with each step the clearance less the margin over the sweep bound, so that no point can be carried through an obstacle.
inline EdgeResult edge_conservative(const ArmCheck& c, const std::vector<double>& a, const std::vector<double>& b, const std::vector<double>& bounds, double margin) {
  EdgeResult out;
  std::vector<double> dq(a.size());
  for (std::size_t i = 0; i < a.size(); ++i) dq[i] = b[i] - a[i];
  const double full = sweep_distance(bounds, dq);  // the sweep of the whole edge
  double u = 0.0;
  while (true) {
    std::vector<double> q(a.size());
    for (std::size_t i = 0; i < a.size(); ++i) q[i] = a[i] + dq[i] * u;
    ++out.checks;
    const double d = clearance_at(c, q);
    if (d < margin) {
      out.free = false;
      return out;
    }
    if (u >= 1.0) return out;
    double du = full > 1e-12 ? (d - margin) / full : 1.0;
    if (du < 1e-4) du = 1e-4;
    u = std::fmin(1.0, u + du);
  }
}


#endif
