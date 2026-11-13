// rc/legged/quad_job.hpp
//
// A quadruped for a job, from lesson 36-08: the capstone of the legged track.
//
// Job is the walk as numbers: the speed wanted, the steepest slope and the highest step on the way, the greatest push (a speed given to the body) it may meet, the extra load carried, the length of the route, the nearest a person comes to the route and the least speed that is useful near one. Robot is the machine as numbers: the leg, the knee motor, the body's mass and the height of its centre, the half wheelbase, the friction of the floor, the leg's spare reach up and down, the swing time and step limits, the lateral reach of a catching step, the battery, the idle power and the person model of 31-06. check_job runs each requirement and keeps its figure: the speed against the fastest the legs allow (36-01), the slope against the least of the slope limits (35-08) and the step against what the servos can swing in the swing time, the push against the largest that can be answered by a step (35-07), the knee's settling temperature in a trot with the load against its limit and the rise from a crouch with the load within the motor's torque and heat (34-06, 36-03), the range on the battery with the power of eight joints' winding loss and the idle power against 1.2 times the route, and the speed that the nearest person allows against the useful minimum. It gives the figures, whether the walk is released and the number of the first requirement that failed (0 none, 1 speed, 2 terrain, 3 push, 4 knees, 5 battery, 6 people).
//
// A walk of 0.6 m/s with 1 kg over 500 m was 1 (1 released): the legs allowed 1.60 m/s, the slope limit was 27.8 degrees and the step limit 0.050 m, a push of up to 0.31 m/s could be answered, the knees settled 39.5 K above the air against 60 K, the range was 957 m and a person allowed 3.48 m/s. A speed of 2.0 m/s failed at requirement 1, a slope of 40 degrees at 2, a push of 0.9 m/s at 3, 30 kg of load at 4, a route of 5000 m at 5 and a person 0.3 m away at 6.

#ifndef RC_RC_LEGGED_QUAD_JOB_HPP
#define RC_RC_LEGGED_QUAD_JOB_HPP

#include <cmath>
#include <rc/arm/limits_heat.hpp>
#include <rc/arm/ssm.hpp>
#include <rc/legged/get_up.hpp>
#include <rc/legged/leg_kin.hpp>
#include <rc/legged/leg_load.hpp>
#include <rc/legged/push_recovery.hpp>
#include <rc/legged/terrain_limits.hpp>
#include <rc/legged/velocity_cmd.hpp>

namespace rc {
namespace legged {

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


}  // namespace legged
}  // namespace rc

#endif
