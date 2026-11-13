#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/arm/limits_heat.hpp>
#include <rc/arm/ssm.hpp>
#include <rc/legged/get_up.hpp>
#include <rc/legged/leg_kin.hpp>
#include <rc/legged/leg_load.hpp>
#include <rc/legged/push_recovery.hpp>
#include <rc/legged/terrain_limits.hpp>
#include <rc/legged/velocity_cmd.hpp>

using namespace rc::legged;
using namespace rc::arm;

using rc::arm::heat_watts;
using rc::arm::JointMotor;
using rc::arm::max_speed;
using rc::arm::SsmParams;

struct Job {
  double speed = 0.6;          // m/s wanted
  double slope_deg = 5.0;      // the steepest slope on the way
  double step_height = 0.02;   // m, the highest step
  double push = 0.15;          // m/s, the greatest push (speed given to the body) it may meet
  double payload = 1.0;        // kg
  double route = 500.0;        // m
  double person_sep = 3.0;     // m, the nearest a person comes
  double min_useful_speed = 0.2;
};

struct Robot {
  LegGeom leg;
  JointMotor motor;
  double mass = 12.0;          // kg
  double com_h = 0.27;         // m
  double half_wheelbase = 0.19;
  double mu = 0.6;
  double spare_up = 0.10, spare_down = 0.12;
  double swing = 0.25;         // s
  double step_max = 0.20;      // m
  double duty = 0.5;
  double period_min = 0.25;    // s
  double reach = 0.25;         // m, how far a catching step can land from the support
  double margin = 0.02;        // m
  double stand_h = 0.28, crouch_h = 0.12;
  double battery_wh = 100.0;
  double idle_w = 15.0;
  double servo_rate = 8.0;
  SsmParams ssm;
};

struct JobVerdict {
  double v_legs = 0.0;
  bool speed_ok = false;
  double a_lim = 0.0;          // degrees, the least of the slope limits
  double s_lim = 0.0;          // m, the step the servos can swing
  bool terrain_ok = false;
  double v_push = 0.0;
  bool push_ok = false;
  double rise = 0.0;           // K, the knee's settling temperature in a trot with the load
  bool getup_ok = false;
  bool knees_ok = false;
  double range = 0.0;          // m
  bool battery_ok = false;
  double v_people = 0.0;
  bool people_ok = false;
  bool released = false;
  int first_fail = 0;          // 0 none, 1 speed, 2 terrain, 3 push, 4 knees, 5 battery, 6 people
};

// Every requirement of the legged track against one walk.
inline JobVerdict check_job(const Robot& r, const Job& j) {
  JobVerdict v;
  const double kDeg = 180.0 / 3.14159265358979323846;
  v.v_legs = max_speed(r.step_max, r.duty, r.period_min);
  v.speed_ok = j.speed <= v.v_legs;
  const double a_f = max_slope_friction(r.mu) * kDeg, a_t = max_slope_tip(r.half_wheelbase, r.com_h) * kDeg, a_l = max_slope_level(r.half_wheelbase, r.spare_up, r.spare_down) * kDeg;
  v.a_lim = a_f < a_t ? (a_f < a_l ? a_f : a_l) : (a_t < a_l ? a_t : a_l);
  v.s_lim = max_step_servo(r.leg, 0.10, r.stand_h, r.swing, 0.03, r.servo_rate);
  v.terrain_ok = j.slope_deg <= v.a_lim && j.step_height <= v.s_lim;
  v.v_push = max_push_speed(r.reach, r.margin, r.swing, r.com_h);
  v.push_ok = j.push <= v.v_push;
  const double m = r.mass + j.payload;
  double tau[3];
  stance_torques(r.leg, r.com_h, m, 2, tau);
  const double rms = cycle_rms_torque(std::fabs(tau[2]), 0.3, r.duty);
  v.rise = winding_rise(r.motor, rms);
  const RisePlan rp = rise_plan(r.leg, m, r.motor, r.crouch_h, r.stand_h, 1.0, 40);
  v.getup_ok = rp.torque_ok && rp.heat_ok;
  v.knees_ok = v.rise <= r.motor.rise_limit && v.getup_ok;
  const double power = 8.0 * heat_watts(r.motor, rms) + r.idle_w;
  v.range = r.battery_wh * 3600.0 / power * j.speed;
  v.battery_ok = v.range >= 1.2 * j.route;
  v.v_people = max_speed(r.ssm, j.person_sep);
  v.people_ok = v.v_people >= j.min_useful_speed;
  const bool ok[6] = {v.speed_ok, v.terrain_ok, v.push_ok, v.knees_ok, v.battery_ok, v.people_ok};
  for (int i = 0; i < 6; ++i)
    if (!ok[i]) {
      v.first_fail = i + 1;
      break;
    }
  v.released = v.first_fail == 0;
  return v;
}


#endif
