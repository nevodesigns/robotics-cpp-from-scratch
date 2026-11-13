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
  (void)r;  // TODO: in this order: the speed against max_speed(step_max, duty, period_min) (36-01); the slope against the least of max_slope_friction, max_slope_tip and max_slope_level (degrees) and the step against max_step_servo (35-08); the push against max_push_speed(reach, margin, swing, com_h) (35-07); the knee's winding_rise in a trot (stance_torques with two feet down at the body's height with mass plus payload, cycle_rms_torque with a swing torque of 0.3) against the motor's rise limit and rise_plan (36-03) for torque and heat; the range from the power (8 times the winding loss plus the idle power) against 1.2 times the route; and the person's max_speed (31-06) against the useful minimum; first_fail is the number of the first that failed
  (void)j;
  return v;
}


#endif
