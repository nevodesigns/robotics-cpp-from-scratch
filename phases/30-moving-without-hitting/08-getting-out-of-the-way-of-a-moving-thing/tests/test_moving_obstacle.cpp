#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "rc/arm/ik_closed.hpp"
#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

std::vector<double> radii() {
  std::vector<double> r(6, 0.05);
  r[0] = 0.08;
  r[1] = 0.06;
  r[3] = 0.05;
  r[5] = 0.04;
  return r;
}

std::vector<double> down_q(const ArmTable& t, double x, double y, double z) {
  Pose p;
  p.r[1][1] = -1.0;
  p.r[2][2] = -1.0;
  p.p[0] = x;
  p.p[1] = y;
  p.p[2] = z;
  const std::vector<std::vector<double> > sols = ik_spherical(t, p, Pose(), trans(0.0, 0.0, 0.10));
  if (sols.empty()) return std::vector<double>(6, 99.0);
  std::vector<double> best = sols[0];
  double bn = 1e9;
  for (std::size_t i = 0; i < sols.size(); ++i) {
    double n = 0.0;
    for (std::size_t k = 0; k < 6; ++k) n += sols[i][k] * sols[i][k];
    if (n < bn) {
      bn = n;
      best = sols[i];
    }
  }
  return best;
}

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

struct Scenario {
  ArmTable table;
  std::vector<double> r, start, goal;
  JointLimits lim;
  double obs_start[3], obs_vel[3];
  double total_time;
  explicit Scenario(double speed) : table(elbow_arm()), r(radii()), lim(limits()) {
    for (std::size_t i = 0; i < table.rows.size(); ++i) {
      table.rows[i].lo = -3.0;
      table.rows[i].hi = 3.0;
    }
    start = down_q(table, 0.40, 0.40, 0.30);
    goal = down_q(table, 0.40, -0.40, 0.30);
    std::vector<std::vector<double> > path(2);
    path[0] = start;
    path[1] = goal;
    const std::vector<TrajSeg> traj = plan_trajectory(path, lim);
    total_time = trajectory_duration(traj);
    std::vector<double> q, qd, qdd, qddd;
    sample_trajectory(traj, 0.5 * total_time, q, qd, qdd, qddd);
    const std::vector<Capsule> mid = arm_capsules(table, q, r, 0.10);
    obs_vel[0] = 0.0;
    obs_vel[1] = 0.0;
    obs_vel[2] = speed;
    for (int k = 0; k < 3; ++k) obs_start[k] = mid.back().b[k] - obs_vel[k] * 0.5 * total_time;
  }
};

}  // namespace

RC_TEST("a track works the velocity out from successive sightings and predicts along it") {
  Track tr;
  double p[3] = {0.0, 0.2, 0.3};
  for (int k = 0; k < 6; ++k) {
    p[0] = 0.5 * 0.1 * k;
    track_update(tr, p, 0.1 * k, 0.7);
  }
  rec("vel_est", tr.v[0], 3);
  rec("vel_true", 0.5, 1);
  RC_CHECK_NEAR(tr.v[0], 0.5, 0.01);
  RC_CHECK_NEAR(tr.v[1], 0.0, 1e-9);
  double ahead[3];
  track_predict(tr, 1.0, ahead);
  RC_CHECK_NEAR(ahead[0], p[0] + tr.v[0], 1e-12);
  Track fresh;
  double q0[3] = {1.0, 2.0, 3.0};
  track_update(fresh, q0, 5.0, 0.7);
  RC_CHECK_NEAR(fresh.v[0], 0.0, 1e-12);
}

RC_TEST("a meeting with an obstacle that is coming is found, about when the geometry says, and one going away is not") {
  const ArmTable t = elbow_arm();
  const std::vector<double> r = radii();
  const std::vector<double> q = down_q(t, 0.40, 0.0, 0.30);
  std::vector<std::vector<double> > hold(2, q);
  hold[1][0] += 1e-6;
  JointLimits lim;
  lim.vmax.assign(6, 2.0);
  lim.amax.assign(6, 4.0);
  lim.jmax.assign(6, 20.0);
  const std::vector<TrajSeg> traj = plan_trajectory(hold, lim);
  const std::vector<Capsule> caps = arm_capsules(t, q, r, 0.10);
  Track tr;
  tr.seen = true;
  tr.t = 0.0;
  for (int k = 0; k < 3; ++k) tr.p[k] = caps.back().b[k];
  tr.p[0] += 1.0;
  tr.v[0] = -0.5;
  const double ttc = time_to_collision(t, r, 0.10, traj, 0.0, tr, 0.0, 0.08, 0.0, 3.0, 0.02);
  rec("ttc", ttc, 2);
  RC_CHECK(ttc > 0.8 && ttc < 2.1);
  tr.v[0] = 0.5;
  RC_CHECK(time_to_collision(t, r, 0.10, traj, 0.0, tr, 0.0, 0.08, 0.0, 3.0, 0.02) < 0.0);
}

RC_TEST("an arm that does not look goes through what rises in its path; one that looks stops, and either waits or goes round, and both are safe") {
  Scenario sc(0.6);
  ReplanConfig blind, wait, plan;
  blind.monitor = false;
  wait.replan = false;
  const ReplanRun a = run_with_moving_obstacle(sc.table, sc.r, 0.10, sc.lim, sc.start, sc.goal, sc.obs_start, sc.obs_vel, 0.08, blind, 3);
  const ReplanRun w = run_with_moving_obstacle(sc.table, sc.r, 0.10, sc.lim, sc.start, sc.goal, sc.obs_start, sc.obs_vel, 0.08, wait, 3);
  const ReplanRun p = run_with_moving_obstacle(sc.table, sc.r, 0.10, sc.lim, sc.start, sc.goal, sc.obs_start, sc.obs_vel, 0.08, plan, 3);
  ReplanConfig free_cfg;
  free_cfg.monitor = false;
  const double far_start[3] = {5.0, 5.0, 5.0};
  const double still[3] = {0.0, 0.0, 0.0};
  const ReplanRun f = run_with_moving_obstacle(sc.table, sc.r, 0.10, sc.lim, sc.start, sc.goal, far_start, still, 0.08, free_cfg, 3);
  rec("clear_blind", a.min_clearance, 3);
  rec("clear_wait", w.min_clearance, 3);
  rec("clear_plan", p.min_clearance, 3);
  rec("t_wait", w.time, 2);
  rec("t_plan", p.time, 2);
  rec("t_free", f.time, 2);
  rec("plans", p.plans, 0);
  RC_CHECK(a.reached && w.reached && p.reached && f.reached);
  RC_CHECK(a.min_clearance < 0.0);
  RC_CHECK(w.min_clearance > 0.0);
  RC_CHECK(p.min_clearance > 0.0);
  RC_CHECK(w.time > f.time);
  RC_CHECK(p.plans >= 1);
  RC_CHECK(p.time > w.time);
}

RC_TEST("the monitor keeps looking after a plan: with no room planned for the obstacle to move into, the arm still gets round with no contact") {
  Scenario sc(0.6);
  ReplanConfig cfg;
  cfg.plan_extra = 0.0;
  const ReplanRun r = run_with_moving_obstacle(sc.table, sc.r, 0.10, sc.lim, sc.start, sc.goal, sc.obs_start, sc.obs_vel, 0.08, cfg, 3);
  RC_CHECK(r.reached);
  RC_CHECK(r.min_clearance > 0.0);
  RC_CHECK(r.plans >= 1);
}

RC_TEST("when the arm is already inside what planning has to avoid there is no plan, and the arm waits") {
  Scenario sc(0.6);
  ReplanConfig cfg;
  cfg.plan_extra = 0.6;
  const ReplanRun r = run_with_moving_obstacle(sc.table, sc.r, 0.10, sc.lim, sc.start, sc.goal, sc.obs_start, sc.obs_vel, 0.08, cfg, 3);
  rec("no_plan", r.waits > 0 ? 1 : 0, 0);
  RC_CHECK(r.waits > 0);
  RC_CHECK(r.min_clearance > 0.0);
  RC_CHECK(r.reached);
}
