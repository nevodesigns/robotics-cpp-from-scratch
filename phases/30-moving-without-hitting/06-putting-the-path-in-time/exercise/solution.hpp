#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

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
  (void)distance;  // TODO: if the acceleration reaches A, distance = P (P / A + A / J), a quadratic in P; otherwise distance = 2 P^1.5 / sqrt(J); take the smaller of that and V (the knee is the speed at which the acceleration just reaches A)
  (void)knee;
  return V;
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


#endif
