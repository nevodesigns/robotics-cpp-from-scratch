// rc/arm/time_param.hpp
//
// Putting the path in time, from lesson 30-06.
//
// JointLimits holds, for each joint, the largest speed, acceleration and jerk it may have. For a straight segment of a path in joint space with a unit direction u, a distance s along it gives joint i a speed u_i times the speed along the segment (and likewise for acceleration and jerk), so the limits along the segment are the smallest over the joints of the joint's limit over the size of its share of u. scurve_plan gives, for a distance and limits along it, the phases of the profile that covers it from rest to rest within them: the jerk goes up to the most, holds that acceleration, comes down, the speed cruises, and the whole is mirrored. When the segment is too short to reach the top speed the peak is found in closed form, in one of two regimes (the acceleration reaches its limit, or not). scurve_at gives the distance, speed, acceleration and jerk at a time. plan_trajectory makes one S curve for each segment of a path, one after the other, at rest at every vertex. sample_trajectory gives the joint angles, speeds, accelerations and jerks at a time. trapezoid_time is the time of the profile with no jerk limit, for comparison.
//
// For a move of 1 rad with limits of 2 rad/s, 4 rad/s2 and 20 rad/s3 the S curve took 1.220 s and the trapezoid 1.000 s, and the trapezoid's jerk, found by differencing, was 4000 rad/s3 against 20. Over a path of 6 segments no joint's speed, acceleration or jerk passed its limit (the largest shares of them were 0.727, 1.000 and 1.000). A wandering path of 17 points took 14.33 s and its straight shortcut 1.45 s. With the speed limits halved a long move took 6.45 s against 3.70.

#ifndef RC_RC_ARM_TIME_PARAM_HPP
#define RC_RC_ARM_TIME_PARAM_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace arm {

struct JointLimits {
  std::vector<double> vmax;
  std::vector<double> amax;
  std::vector<double> jmax;
};

struct SCurve {
  double distance = 0.0;
  double peak = 0.0;   // the speed it cruises at, or reaches
  double jerk = 0.0;   // the jerk used
  double tj = 0.0;     // time of each jerk phase
  double ta = 0.0;     // time of the constant acceleration phase (zero if the acceleration does not reach its limit)
  double tc = 0.0;     // time of the cruise
  double total = 0.0;
};

// The speed a move of a distance can reach when it is too short to reach V (rest to rest, within A and J).
inline double scurve_peak(double distance, double V, double A, double J) {
  const double knee = A * A / J;  // the speed at which the acceleration just reaches its limit
  // If the acceleration reaches A: distance = P (P / A + A / J).
  const double p = 0.5 * A * (-A / J + std::sqrt((A / J) * (A / J) + 4.0 * distance / A));
  if (p >= knee) return std::fmin(V, p);
  // Otherwise the acceleration is a triangle: distance = 2 P^1.5 / sqrt(J).
  return std::fmin(V, std::pow(0.5 * distance * std::sqrt(J), 2.0 / 3.0));
}

// The phases of the profile that covers a distance from rest to rest within a speed, acceleration and jerk.
inline SCurve scurve_plan(double distance, double V, double A, double J) {
  SCurve c;
  c.distance = distance;
  c.jerk = J;
  if (distance <= 0.0) return c;
  const double peak = scurve_peak(distance, V, A, J);
  c.peak = peak;
  const double knee = A * A / J;
  const double ap = peak >= knee ? A : std::sqrt(peak * J);
  c.tj = ap / J;
  c.ta = peak >= knee ? (peak - knee) / A : 0.0;
  const double t_acc = 2.0 * c.tj + c.ta;
  const double d_acc_dec = peak * t_acc;  // accelerating and decelerating, each over t_acc at a mean speed of peak / 2
  c.tc = distance > d_acc_dec ? (distance - d_acc_dec) / peak : 0.0;
  c.total = 2.0 * t_acc + c.tc;
  return c;
}

// The distance, speed, acceleration and jerk a time into the profile.
inline void scurve_at(const SCurve& c, double t, double& s, double& v, double& a, double& j) {
  const double dur[7] = {c.tj, c.ta, c.tj, c.tc, c.tj, c.ta, c.tj};
  const double jer[7] = {c.jerk, 0.0, -c.jerk, 0.0, -c.jerk, 0.0, c.jerk};
  s = 0.0;
  v = 0.0;
  a = 0.0;
  j = 0.0;
  if (t <= 0.0) return;
  double left = t;
  for (int k = 0; k < 7; ++k) {
    const double dt = left < dur[k] ? left : dur[k];
    s += v * dt + a * dt * dt / 2.0 + jer[k] * dt * dt * dt / 6.0;
    v += a * dt + jer[k] * dt * dt / 2.0;
    a += jer[k] * dt;
    j = jer[k];
    left -= dt;
    if (left <= 0.0) {
      if (dt >= dur[k]) {
        j = k + 1 < 7 ? jer[k + 1] : 0.0;
      }
      return;
    }
  }
  s = c.distance;
  v = 0.0;
  a = 0.0;
  j = 0.0;
}

// The time of the profile with no limit on jerk: a trapezoid of speed.
inline double trapezoid_time(double distance, double V, double A) {
  if (distance <= 0.0) return 0.0;
  if (distance >= V * V / A) return distance / V + V / A;
  return 2.0 * std::sqrt(distance / A);
}

struct TrajSeg {
  std::vector<double> from;
  std::vector<double> u;  // the unit direction in joint space
  SCurve sc;
  double t0 = 0.0;
};

// One S curve for each segment of the path, one after the other, at rest at every vertex; the limits along a segment are the smallest of each joint's limit over its share of the direction.
inline std::vector<TrajSeg> plan_trajectory(const std::vector<std::vector<double> >& path, const JointLimits& lim) {
  std::vector<TrajSeg> out;
  double t = 0.0;
  for (std::size_t i = 1; i < path.size(); ++i) {
    TrajSeg seg;
    seg.from = path[i - 1];
    double d2 = 0.0;
    for (std::size_t k = 0; k < path[i].size(); ++k) d2 += (path[i][k] - path[i - 1][k]) * (path[i][k] - path[i - 1][k]);
    const double dist = std::sqrt(d2);
    if (dist < 1e-12) continue;
    seg.u.resize(path[i].size());
    double V = 1e18, A = 1e18, J = 1e18;
    for (std::size_t k = 0; k < seg.u.size(); ++k) {
      seg.u[k] = (path[i][k] - path[i - 1][k]) / dist;
      const double share = std::fabs(seg.u[k]);
      if (share > 1e-9) {
        V = std::fmin(V, lim.vmax[k] / share);
        A = std::fmin(A, lim.amax[k] / share);
        J = std::fmin(J, lim.jmax[k] / share);
      }
    }
    seg.sc = scurve_plan(dist, V, A, J);
    seg.t0 = t;
    t += seg.sc.total;
    out.push_back(seg);
  }
  return out;
}

inline double trajectory_duration(const std::vector<TrajSeg>& traj) { return traj.empty() ? 0.0 : traj.back().t0 + traj.back().sc.total; }

// The joint angles, speeds, accelerations and jerks at a time.
inline void sample_trajectory(const std::vector<TrajSeg>& traj, double t, std::vector<double>& q, std::vector<double>& qd, std::vector<double>& qdd, std::vector<double>& qddd) {
  if (traj.empty()) return;
  std::size_t k = 0;
  while (k + 1 < traj.size() && t >= traj[k + 1].t0) ++k;
  const TrajSeg& g = traj[k];
  double s, v, a, j;
  scurve_at(g.sc, t - g.t0, s, v, a, j);
  const std::size_t n = g.from.size();
  q.assign(n, 0.0);
  qd.assign(n, 0.0);
  qdd.assign(n, 0.0);
  qddd.assign(n, 0.0);
  for (std::size_t i = 0; i < n; ++i) {
    q[i] = g.from[i] + g.u[i] * s;
    qd[i] = g.u[i] * v;
    qdd[i] = g.u[i] * a;
    qddd[i] = g.u[i] * j;
  }
}


}  // namespace arm
}  // namespace rc

#endif
