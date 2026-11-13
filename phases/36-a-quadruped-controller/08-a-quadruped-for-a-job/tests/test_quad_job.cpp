#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

Robot make_robot() {
  Robot r;
  r.motor.kt_out = 1.0;
  r.motor.resistance = 0.5;
  r.motor.r_th = 1.5;
  r.motor.c_th = 60.0;
  r.motor.rise_limit = 60.0;
  r.motor.tau_peak = 33.0;
  r.motor.tau_cont = 10.0;
  return r;
}

}  // namespace

RC_TEST("a walk inside what the robot can do is released, with every figure kept") {
  const Robot r = make_robot();
  const Job j;
  const JobVerdict v = check_job(r, j);
  rec("speed", j.speed, 1);
  rec("payload", j.payload, 0);
  rec("route", j.route, 0);
  rec("rel_ok", v.released ? 1 : 0, 0);
  rec("v_legs", v.v_legs, 2);
  rec("a_lim", v.a_lim, 1);
  rec("s_lim", v.s_lim, 3);
  rec("v_push", v.v_push, 2);
  rec("rise", v.rise, 1);
  rec("rise_limit", r.motor.rise_limit, 0);
  rec("range_m", v.range, 0);
  rec("v_people", v.v_people, 2);
  RC_CHECK(v.speed_ok && v.terrain_ok && v.push_ok && v.knees_ok && v.battery_ok && v.people_ok);
  RC_CHECK(v.released && v.first_fail == 0);
}

RC_TEST("each way a walk can be too much is named by the first requirement that fails") {
  const Robot r = make_robot();
  Job fast;
  fast.speed = 2.0;
  const JobVerdict a = check_job(r, fast);
  rec("v_fast", fast.speed, 1);
  rec("first_speed", a.first_fail, 0);
  RC_CHECK(a.first_fail == 1);
  Job steep;
  steep.slope_deg = 40.0;
  const JobVerdict b = check_job(r, steep);
  rec("slope_steep", steep.slope_deg, 0);
  rec("first_slope", b.first_fail, 0);
  RC_CHECK(b.first_fail == 2);
  Job step;
  step.step_height = 0.15;
  RC_CHECK(check_job(r, step).first_fail == 2);
  Job shove;
  shove.push = 0.9;
  const JobVerdict c = check_job(r, shove);
  rec("push_big", shove.push, 1);
  rec("first_push", c.first_fail, 0);
  RC_CHECK(c.first_fail == 3);
  Job heavy;
  heavy.payload = 30.0;
  const JobVerdict d = check_job(r, heavy);
  rec("payload_heavy", heavy.payload, 0);
  rec("first_load", d.first_fail, 0);
  RC_CHECK(d.first_fail == 4);
  Job far_route;
  far_route.route = 5000.0;
  const JobVerdict e = check_job(r, far_route);
  rec("route_long", far_route.route, 0);
  rec("first_route", e.first_fail, 0);
  RC_CHECK(e.first_fail == 5);
  Job crowded;
  crowded.person_sep = 0.3;
  const JobVerdict f = check_job(r, crowded);
  rec("sep_close", crowded.person_sep, 1);
  rec("first_people", f.first_fail, 0);
  RC_CHECK(f.first_fail == 6);
}

RC_TEST("the first failure is the first in the list, and the figures of the others are still kept") {
  const Robot r = make_robot();
  Job j;
  j.speed = 2.0;
  j.slope_deg = 40.0;
  j.push = 0.9;
  const JobVerdict v = check_job(r, j);
  RC_CHECK(v.first_fail == 1);
  RC_CHECK(!v.speed_ok && !v.terrain_ok && !v.push_ok);
  RC_CHECK(v.v_push > 0.0 && v.a_lim > 0.0);
  RC_CHECK(!v.released);
}
