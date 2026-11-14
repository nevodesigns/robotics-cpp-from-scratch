#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include "rc/arm/ik_closed.hpp"
#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"hold_tq", 37.65897309000464},
    {"fold_tq", 2.0817958450804968},
    {"t_hold", 293.08485538908985},
    {"pay_out", 2.4954187255970233},
    {"pay_in", 10.0},
    {"t_peak", 0.2854598842650375},
    {"t_cont", 0.38816841828065657},
    {"quad", 4.000978441304984},
    {"rise0", 117.68852113204068},
    {"rise4", 27.23011006778514},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

ArmModel typical() {
  ArmModel m;
  m.table = elbow_arm();
  std::vector<double> masses(6);
  masses[0] = 3.0;
  masses[1] = 4.0;
  masses[2] = 2.0;
  masses[3] = 2.0;
  masses[4] = 1.0;
  masses[5] = 0.5;
  m.links = rod_links(m.table, masses, 0.04);
  return m;
}

// Motors for the six joints: the big ones at the shoulder, small ones at the wrist.
std::vector<JointMotor> motors() {
  std::vector<JointMotor> v(6);
  const double scale[6] = {1.0, 1.0, 0.6, 0.3, 0.2, 0.12};
  for (int i = 0; i < 6; ++i) {
    v[static_cast<std::size_t>(i)].kt_out = 4.0 * scale[i];
    v[static_cast<std::size_t>(i)].tau_peak = 150.0 * scale[i];
    v[static_cast<std::size_t>(i)].tau_cont = 60.0 * scale[i];
    v[static_cast<std::size_t>(i)].c_th = 200.0 * scale[i];
  }
  return v;
}

std::vector<double> limits_of(const std::vector<JointMotor>& m) {
  std::vector<double> l;
  for (std::size_t i = 0; i < m.size(); ++i) l.push_back(m[i].tau_cont);
  return l;
}

// A reach out over the table and a fold, in joint angles.
std::vector<double> stretched() {
  std::vector<double> q(6, 0.0);
  q[1] = 0.0;
  q[2] = 1.2;
  return q;
}
std::vector<double> folded() {
  std::vector<double> q(6, 0.0);
  q[1] = 1.5;
  q[2] = -1.5;
  return q;
}

}  // namespace

RC_TEST("holding the arm out heats a joint to its limit in minutes and holding it folded never does") {
  const ArmModel m = typical();
  const std::vector<JointMotor> mo = motors();
  const double out_tq = std::fabs(gravity_torques(m, stretched())[1]);
  const double in_tq = std::fabs(gravity_torques(m, folded())[1]);
  const JointMotor shoulder = mo[1];
  rec("hold_tq", out_tq, 1);
  rec("fold_tq", in_tq, 1);
  const double t_out = time_to_limit(shoulder, out_tq);
  rec("t_hold", t_out, 0);
  RC_CHECK(out_tq > 3.0 * in_tq);
  RC_CHECK(t_out > 10.0 && t_out < 3600.0);
  RC_CHECK(time_to_limit(shoulder, in_tq) > 1e17);
  // The closed form against stepping the lump.
  const double p = heat_watts(shoulder, out_tq);
  double T = 0.0, t = 0.0;
  while (T < shoulder.rise_limit && t < 20000.0) {
    T += (p - T / shoulder.r_th) / shoulder.c_th * 0.01;
    t += 0.01;
  }
  RC_CHECK_NEAR(t, t_out, 0.02 * t_out);
}

RC_TEST("the payload that fits depends on the pose, and a payload at the limit is held at the limit") {
  const ArmModel m = typical();
  const std::vector<double> lim = limits_of(motors());
  const Pose tool = trans(0.0, 0.0, 0.10);
  const double out = max_payload(m, stretched(), tool, lim, 10.0);
  const double in = max_payload(m, folded(), tool, lim, 10.0);
  rec("pay_out", out, 2);
  rec("pay_in", in, 2);
  RC_CHECK(out >= 0.0 && in > out + 0.5);
  RC_CHECK_NEAR(in, 10.0, 1e-9);
  // At exactly that mass some joint is at its limit, and at a little more it is over.
  const double at[3] = {0.0, 0.0, 0.10};
  const std::vector<double> t_at = gravity_torques(with_payload(m, out, at), stretched());
  const std::vector<double> t_over = gravity_torques(with_payload(m, out * 1.05 + 0.01, at), stretched());
  bool touching = false, over = false;
  for (std::size_t i = 0; i < 6; ++i) {
    if (std::fabs(std::fabs(t_at[i]) - lim[i]) < 0.05) touching = true;
    if (std::fabs(t_over[i]) > lim[i]) over = true;
  }
  RC_CHECK(touching);
  RC_CHECK(over);
}

RC_TEST("the shortest move is shorter by the peak limits than by the continuous ones, and a faster move asks for much more torque") {
  const ArmModel m = typical();
  Quintic mv;
  mv.q0.assign(6, 0.0);
  mv.q1.assign(6, 0.0);
  mv.q0[1] = 0.3;
  mv.q1[1] = 1.0;
  mv.q0[2] = 0.5;
  mv.q1[2] = 1.2;
  mv.duration = 1.0;
  const std::vector<JointMotor> mo = motors();
  const double t_peak = shortest_move_time(m, mv, mo, true);
  const double t_cont = shortest_move_time(m, mv, mo, false);
  rec("t_peak", t_peak, 2);
  rec("t_cont", t_cont, 2);
  RC_CHECK(t_peak > 0.0 && t_cont > 0.0);
  RC_CHECK(t_cont > t_peak);
  // Torque of the inertia part: zero the weight by comparing two durations of the same move.
  Quintic a = mv, b = mv;
  a.duration = 1.0;
  b.duration = 0.5;
  const MoveTorques ta = move_torques(m, a), tb = move_torques(m, b);
  Quintic slow = mv;
  slow.duration = 60.0;
  const MoveTorques hold = move_torques(m, slow);
  const double extra_a = ta.peak[1] - hold.peak[1], extra_b = tb.peak[1] - hold.peak[1];
  rec("quad", extra_b / extra_a, 1);
  RC_CHECK(extra_b > 2.5 * extra_a);
}

RC_TEST("repeating a move with a wait lets the motor cool, and a move that cannot be repeated at once can be repeated with a wait") {
  const ArmModel m = typical();
  Quintic mv;
  mv.q0.assign(6, 0.0);
  mv.q1.assign(6, 0.0);
  mv.q0[1] = 0.0;
  mv.q1[1] = 1.4;
  mv.q0[2] = 1.2;
  mv.q1[2] = -1.3;
  mv.duration = 1.0;
  JointMotor hot = motors()[1];
  hot.kt_out = 3.0;
  const double r0 = duty_rise(m, mv, hot, 1, 0.0), r4 = duty_rise(m, mv, hot, 1, 4.0), r30 = duty_rise(m, mv, hot, 1, 30.0);
  rec("rise0", r0, 0);
  rec("rise4", r4, 0);
  RC_CHECK(r0 > r4 && r4 > r30);
  RC_CHECK(r0 > hot.rise_limit);
  RC_CHECK(r30 < hot.rise_limit);
}

RC_TEST("a payload that the arm alone cannot hold is refused with a negative answer") {
  const ArmModel m = typical();
  std::vector<double> tiny(6, 0.5);
  RC_CHECK(max_payload(m, stretched(), trans(0.0, 0.0, 0.10), tiny, 10.0) < 0.0);
}
