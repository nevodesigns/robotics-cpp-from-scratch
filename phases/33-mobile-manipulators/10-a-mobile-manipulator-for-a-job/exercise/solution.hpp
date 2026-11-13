#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/mobile/base_reach.hpp>
#include <rc/mobile/grasp_budget.hpp>
#include <rc/mobile/mm_supervisor.hpp>
#include <rc/mobile/parking.hpp>
#include <rc/mobile/payload_effect.hpp>
#include <rc/mobile/tip_margin.hpp>
#include <vector>

using namespace rc::arm;
using namespace rc::mobile;

using rc::arm::ArmModel;
using rc::arm::SsmParams;
using rc::arm::max_speed;

struct Job {
  double pick[3] = {2.0, 0.5, 0.20};
  double place[3] = {0.5, 2.0, 0.20};
  double payload = 2.0;          // kg
  double route = 6.0;            // m
  double speed_wanted = 0.8;     // m/s
  double person_sep = 3.0;       // m, the nearest a person comes to the route
  double stop_sigma = 0.03;      // m, how well the base stops
  double min_useful_speed = 0.3; // m/s
  std::vector<Obstacle> obstacles;
};

struct Robot {
  MobileManipulator m;
  std::vector<double> q_tuck;
  std::vector<double> q_reach;
  MassModel masses;
  ArmModel model;
  std::vector<double> joint_limits;
  double rated = 5.0;
  BaseDrive drive;
  Requirements req;
  Sigmas sigmas;
  double camera_sigma = 0.002;
  double grip_tol = 0.010;
  SsmParams ssm;
  ParkParams park;
  double tip_margin = 0.8;
};

struct JobVerdict {
  bool park_pick = false, park_place = false;
  double tolerance = 0.0;      // m, of the ring at the pick
  bool stop_ok = false;
  double budget = 0.0;         // kg
  bool payload_ok = false;
  double p_grasp = 0.0;
  bool grasp_ok = false;
  double range = 0.0;          // m
  bool battery_ok = false;
  double v_load = 0.0;         // m/s the load allows
  bool speed_ok = false;
  double v_people = 0.0;       // m/s the nearest person allows
  bool people_ok = false;
  bool released = false;
  int first_fail = 0;          // 0 none, 1 parking, 2 stopping, 3 payload, 4 grasp, 5 battery, 6 travel speed, 7 people
};

// Every requirement of the phase against one job.
inline JobVerdict check_job(const Robot& r, const Job& j) {
  JobVerdict v;
  (void)r;  // TODO: run, in this order, the parking choice at the pick and the place (reach_annulus at the target's height in the arm's frame, then choose_park); the ring's tolerance against three times the stopping sigma; payload_budget against the payload; the grasp chance with the camera (sigma_after_servo, grasp_probability) at least 0.99; range_m for the loaded base against 1.2 times the route; tip_speed_limit from the tipping deceleration of the tucked pose with the payload against the speed wanted; max_speed of the person's distance against the useful minimum; first_fail is the number of the first requirement that failed
  (void)j;
  return v;
}


#endif
