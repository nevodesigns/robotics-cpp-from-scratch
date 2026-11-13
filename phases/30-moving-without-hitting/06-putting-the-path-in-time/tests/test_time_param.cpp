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

JointLimits limits() {
  JointLimits l;
  const double v[6] = {2.0, 2.0, 2.5, 3.0, 3.0, 4.0}, a[6] = {4.0, 4.0, 5.0, 8.0, 8.0, 10.0}, j[6] = {20.0, 20.0, 25.0, 50.0, 50.0, 60.0};
  for (int i = 0; i < 6; ++i) {
    l.vmax.push_back(v[i]);
    l.amax.push_back(a[i]);
    l.jmax.push_back(j[i]);
  }
  return l;
}

std::vector<std::vector<double> > wandering(int n) {
  std::vector<std::vector<double> > p;
  for (int k = 0; k < n; ++k) {
    std::vector<double> q(6, 0.0);
    q[0] = 1.5 * k / (n - 1) + 0.3 * std::sin(2.5 * k);
    q[1] = 0.8 * k / (n - 1) + 0.3 * std::cos(1.9 * k);
    q[2] = 0.5 * std::sin(1.1 * k);
    p.push_back(q);
  }
  p.front()[0] = 0.0;
  p.front()[1] = 0.3;
  p.front()[2] = 0.0;
  p.back()[0] = 1.5;
  p.back()[1] = 1.1;
  p.back()[2] = 0.5 * std::sin(1.1 * (n - 1));
  return p;
}

}  // namespace

RC_TEST("an S curve covers the distance from rest to rest within its limits, long or short") {
  const double V = 2.0, A = 4.0, J = 20.0;
  const double ds[5] = {0.02, 0.1, 0.4, 1.0, 3.0};
  for (int k = 0; k < 5; ++k) {
    const SCurve c = scurve_plan(ds[k], V, A, J);
    double s, v, a, j;
    scurve_at(c, c.total, s, v, a, j);
    RC_CHECK_NEAR(s, ds[k], 1e-9);
    RC_CHECK_NEAR(v, 0.0, 1e-9);
    RC_CHECK_NEAR(a, 0.0, 1e-9);
    double vm = 0.0, am = 0.0, jm = 0.0;
    for (int i = 0; i <= 2000; ++i) {
      scurve_at(c, c.total * i / 2000.0, s, v, a, j);
      vm = std::fmax(vm, std::fabs(v));
      am = std::fmax(am, std::fabs(a));
      jm = std::fmax(jm, std::fabs(j));
    }
    RC_CHECK(vm <= V + 1e-9);
    RC_CHECK(am <= A + 1e-9);
    RC_CHECK(jm <= J + 1e-9);
  }
  // A move of 3 rad cruises at V; one of 0.02 rad never reaches the acceleration limit.
  RC_CHECK_NEAR(scurve_plan(3.0, V, A, J).peak, V, 1e-12);
  RC_CHECK(scurve_plan(3.0, V, A, J).tc > 0.5);
  RC_CHECK(scurve_plan(0.02, V, A, J).ta == 0.0);
  RC_CHECK(scurve_plan(0.02, V, A, J).peak < V);
}

RC_TEST("the S curve is a little slower than the trapezoid, and the trapezoid asks for a jerk that is nothing but a spike") {
  const double V = 2.0, A = 4.0, J = 20.0;
  const SCurve c = scurve_plan(1.0, V, A, J);
  const double tt = trapezoid_time(1.0, V, A);
  rec("t_s", c.total, 3);
  rec("t_trap", tt, 3);
  RC_CHECK(c.total > tt);
  RC_CHECK(c.total < tt + 2.0 * A / J + 1e-9);
  // The trapezoid's acceleration steps from 0 to A in no time: over a step of a millisecond the jerk is A / 0.001.
  const double jerk = A / 0.001;
  rec("jerk_trap", jerk, 0);
  RC_CHECK(jerk > 100.0 * J);
}

RC_TEST("over a path no joint's speed, acceleration or jerk passes its limit, and the limiting joint is close to it") {
  const std::vector<std::vector<double> > path = wandering(7);
  const JointLimits lim = limits();
  const std::vector<TrajSeg> traj = plan_trajectory(path, lim);
  rec("n_seg", static_cast<double>(traj.size()), 0);
  double rv = 0.0, ra = 0.0, rj = 0.0;
  std::vector<double> q, qd, qdd, qddd;
  const double T = trajectory_duration(traj);
  for (int i = 0; i <= 4000; ++i) {
    sample_trajectory(traj, T * i / 4000.0, q, qd, qdd, qddd);
    for (std::size_t k = 0; k < 6; ++k) {
      rv = std::fmax(rv, std::fabs(qd[k]) / lim.vmax[k]);
      ra = std::fmax(ra, std::fabs(qdd[k]) / lim.amax[k]);
      rj = std::fmax(rj, std::fabs(qddd[k]) / lim.jmax[k]);
    }
  }
  rec("r_v", rv, 3);
  rec("r_a", ra, 3);
  rec("r_j", rj, 3);
  RC_CHECK(rv <= 1.0 + 1e-9 && ra <= 1.0 + 1e-9 && rj <= 1.0 + 1e-9);
  RC_CHECK(rv > 0.9 || ra > 0.9 || rj > 0.9);
  // It starts and ends where the path does, and at rest.
  sample_trajectory(traj, 0.0, q, qd, qdd, qddd);
  for (std::size_t k = 0; k < 6; ++k) RC_CHECK_NEAR(q[k], path.front()[k], 1e-9);
  sample_trajectory(traj, T, q, qd, qdd, qddd);
  for (std::size_t k = 0; k < 6; ++k) {
    RC_CHECK_NEAR(q[k], path.back()[k], 1e-6);
    RC_CHECK_NEAR(qd[k], 0.0, 1e-6);
  }
}

RC_TEST("a path with many vertices stops at each of them, and its straight shortcut does not") {
  const std::vector<std::vector<double> > wander = wandering(17);
  std::vector<std::vector<double> > direct;
  direct.push_back(wander.front());
  direct.push_back(wander.back());
  const JointLimits lim = limits();
  const double tw = trajectory_duration(plan_trajectory(wander, lim)), td = trajectory_duration(plan_trajectory(direct, lim));
  rec("n_pts", 17, 0);
  rec("t_wander", tw, 2);
  rec("t_direct", td, 2);
  RC_CHECK(tw > 2.0 * td);
}

RC_TEST("halving the speed limits nearly doubles the time of a long move and changes a short one much less") {
  std::vector<std::vector<double> > longm, shortm;
  longm.push_back(std::vector<double>(6, 0.0));
  longm.push_back(std::vector<double>(6, 0.0));
  longm.back()[0] = 6.0;
  shortm.push_back(std::vector<double>(6, 0.0));
  shortm.push_back(std::vector<double>(6, 0.0));
  shortm.back()[0] = 0.05;
  JointLimits full = limits(), half = limits();
  for (std::size_t i = 0; i < 6; ++i) half.vmax[i] *= 0.5;
  const double tf = trajectory_duration(plan_trajectory(longm, full)), th = trajectory_duration(plan_trajectory(longm, half));
  rec("t_full", tf, 2);
  rec("t_half", th, 2);
  RC_CHECK(th > 1.6 * tf);
  const double sf = trajectory_duration(plan_trajectory(shortm, full)), sh = trajectory_duration(plan_trajectory(shortm, half));
  RC_CHECK_NEAR(sf, sh, 1e-9);
}
