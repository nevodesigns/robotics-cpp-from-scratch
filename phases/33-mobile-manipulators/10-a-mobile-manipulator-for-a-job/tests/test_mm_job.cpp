#include <rc/test/rc_test.hpp>

#include <cmath>

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

std::vector<double> pose_q(const MobileManipulator& m, double x, double y, double z) {
  Pose p;
  p.r[1][1] = -1.0;
  p.r[2][2] = -1.0;
  p.p[0] = x;
  p.p[1] = y;
  p.p[2] = z;
  const std::vector<std::vector<double> > sols = ik_spherical(m.arm, p, Pose(), m.tool);
  if (sols.empty()) return std::vector<double>(6, 0.0);
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

Robot make_robot() {
  Robot r;
  r.m.arm = elbow_arm();
  for (std::size_t i = 0; i < r.m.arm.rows.size(); ++i) {
    r.m.arm.rows[i].lo = -3.0;
    r.m.arm.rows[i].hi = 3.0;
  }
  r.m.mount = trans(0.0, 0.0, 0.10);
  r.m.tool = trans(0.0, 0.0, 0.10);
  r.q_tuck = pose_q(r.m, 0.20, 0.0, 0.35);
  r.q_reach = pose_q(r.m, 0.65, 0.0, 0.30);
  const double l[6] = {3.0, 2.5, 2.0, 0.8, 0.6, 0.4};
  r.masses.link_mass.assign(l, l + 6);
  r.model.table = r.m.arm;
  r.model.links = rod_links(r.m.arm, std::vector<double>(l, l + 6), 0.04);
  const double lim[6] = {60.0, 60.0, 30.0, 10.0, 8.0, 5.0};
  r.joint_limits.assign(lim, lim + 6);
  return r;
}

}  // namespace

RC_TEST("a job inside what the robot can do is released, with every figure kept") {
  const Robot r = make_robot();
  Job j;
  const JobVerdict v = check_job(r, j);
  rec("payload_ok", j.payload, 0);
  rec("route", j.route, 0);
  rec("rel_ok", v.released ? 1 : 0, 0);
  rec("tol_cm", v.tolerance * 100.0, 0);
  rec("sig_cm", j.stop_sigma * 100.0, 0);
  rec("budget", v.budget, 1);
  rec("p_grasp", v.p_grasp, 3);
  rec("range_km", v.range / 1000.0, 1);
  RC_CHECK(v.park_pick && v.park_place);
  RC_CHECK(v.stop_ok && v.payload_ok && v.grasp_ok && v.battery_ok && v.speed_ok && v.people_ok);
  RC_CHECK(v.released && v.first_fail == 0);
  RC_CHECK(v.tolerance > 0.2);
  RC_CHECK(v.budget >= j.payload);
}

RC_TEST("each way a job can be too much for the robot is named by the requirement that fails first") {
  const Robot r = make_robot();
  Job heavy;
  heavy.payload = 8.0;
  const JobVerdict vh = check_job(r, heavy);
  rec("payload_heavy", heavy.payload, 0);
  rec("first_heavy", vh.first_fail, 0);
  RC_CHECK(vh.first_fail == 3 && !vh.released);
  Job sloppy;
  sloppy.stop_sigma = 0.15;
  const JobVerdict vs = check_job(r, sloppy);
  rec("sig_bad_cm", sloppy.stop_sigma * 100.0, 0);
  rec("first_stop", vs.first_fail, 0);
  RC_CHECK(vs.first_fail == 2);
  Robot blind = r;
  blind.camera_sigma = 0.012;
  const JobVerdict vb = check_job(blind, Job());
  rec("cam_bad_mm", blind.camera_sigma * 1000.0, 0);
  rec("first_cam", vb.first_fail, 0);
  RC_CHECK(vb.first_fail == 4);
  Job far_route;
  far_route.route = 30000.0;
  const JobVerdict vr = check_job(r, far_route);
  rec("route_long", far_route.route, 0);
  rec("first_route", vr.first_fail, 0);
  RC_CHECK(vr.first_fail == 5);
  Job crowded;
  crowded.person_sep = 0.45;
  const JobVerdict vp = check_job(r, crowded);
  rec("sep_close", crowded.person_sep, 2);
  rec("first_people", vp.first_fail, 0);
  RC_CHECK(vp.first_fail == 7);
  Job fast;
  fast.speed_wanted = 3.0;
  const JobVerdict vf = check_job(r, fast);
  rec("first_fast", vf.first_fail, 0);
  RC_CHECK(vf.first_fail == 6);
}

RC_TEST("a job with the shelf out of the reach of any parking place is refused at the first requirement") {
  const Robot r = make_robot();
  Job j;
  std::vector<Obstacle> all;
  for (int i = 0; i < 24; ++i) {
    Obstacle o;
    o.x = j.pick[0] + 0.55 * std::cos(i * 0.2617993877991494);
    o.y = j.pick[1] + 0.55 * std::sin(i * 0.2617993877991494);
    o.r = 0.9;
    all.push_back(o);
  }
  j.obstacles = all;
  const JobVerdict v = check_job(r, j);
  RC_CHECK(!v.park_pick);
  RC_CHECK(v.first_fail == 1 && !v.released);
}
