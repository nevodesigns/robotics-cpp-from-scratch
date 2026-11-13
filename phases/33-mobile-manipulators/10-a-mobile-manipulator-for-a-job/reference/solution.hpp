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
  const BasePose here;
  const double pz = r.m.mount.p[2];
  const Annulus ring_pick = reach_annulus(r.m, j.pick[2] - pz, 0.01);
  const Annulus ring_place = reach_annulus(r.m, j.place[2] - pz, 0.01);
  const double tp[2] = {j.pick[0], j.pick[1]}, tq[2] = {j.place[0], j.place[1]};
  const Obstacle* obs = j.obstacles.empty() ? nullptr : &j.obstacles[0];
  const int no = static_cast<int>(j.obstacles.size());
  v.park_pick = choose_park(r.m, ring_pick, tp, here, obs, no, r.park).found;
  v.park_place = choose_park(r.m, ring_place, tq, here, obs, no, r.park).found;
  const double tol_p = park_tolerance(ring_pick), tol_q = park_tolerance(ring_place);
  v.tolerance = tol_p < tol_q ? tol_p : tol_q;
  v.stop_ok = v.tolerance >= 3.0 * j.stop_sigma;
  const Budget b = payload_budget(r.m, here, r.q_reach, r.masses, r.model, r.joint_limits, r.rated, r.drive, r.req);
  v.budget = b.best;
  v.payload_ok = j.payload <= b.best;
  v.p_grasp = grasp_probability(sigma_after_servo(r.camera_sigma, r.sigmas.arm), r.grip_tol);
  v.grasp_ok = v.p_grasp >= 0.99;
  double fixed = r.masses.base_mass;
  for (std::size_t i = 0; i < r.masses.link_mass.size(); ++i) fixed += r.masses.link_mass[i];
  v.range = range_m(r.drive, fixed + j.payload, j.speed_wanted);
  v.battery_ok = v.range >= 1.2 * j.route;
  MassModel loaded = r.masses;
  loaded.payload = j.payload;
  double com[3];
  com_world(r.m, here, r.q_tuck, loaded, com);
  const TipLimits t = tip_limits(here, com, r.req.half_wheelbase, r.req.half_track);
  v.v_load = tip_speed_limit(t.brake, r.tip_margin, r.req.stop_dist);
  v.speed_ok = v.v_load >= j.speed_wanted;
  v.v_people = max_speed(r.ssm, j.person_sep);
  v.people_ok = v.v_people >= j.min_useful_speed;
  const bool ok[7] = {v.park_pick && v.park_place, v.stop_ok, v.payload_ok, v.grasp_ok, v.battery_ok, v.speed_ok, v.people_ok};
  v.first_fail = 0;
  for (int i = 0; i < 7; ++i)
    if (!ok[i]) {
      v.first_fail = i + 1;
      break;
    }
  v.released = v.first_fail == 0;
  return v;
}


#endif
