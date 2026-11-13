// rc/mobile/mm_job.hpp
//
// A mobile manipulator for a job, from lesson 33-10: the capstone of phase 33.
//
// Job is the work as numbers: the pick and the place (x, y, z in the world), the payload, the length of the route and the speed wanted on it, the nearest a person comes (a separation), the standard deviation of the base's stopping place, and the obstacles. Robot is the machine as numbers: the arm on its base, the poses used for carrying (tucked) and for reaching, the masses, the arm's model and joint limits, the drive, the errors of the base's localisation, the camera's error at working range, the tolerance of the grasp and the person model of 31-06. check_job runs each requirement in turn and keeps its figure: ring and parking at the pick and at the place (33-03), the stopping accuracy against the tolerance of the ring (the tolerance must be at least three times the stopping sigma), the payload budget (33-06), the grasp chance with the camera (33-07, at least 0.99), the range on the battery for the route with the load (at least 1.2 times the route), the travel speed the load allows (33-05 and 33-09) against the speed wanted, and the speed the nearest person allows (31-06) against a useful minimum. It gives the figures, whether the job is released and the number of the first requirement that failed (0 none, 1 parking, 2 stopping, 3 payload, 4 grasp, 5 battery, 6 travel speed, 7 people).
//
// A job of 2 kg over 6 m was 1 (1 released): the tolerance of the ring was 35 cm against a stopping sigma of 3 cm, the payload budget 4.3 kg, the grasp chance 1.000 and the range 13.1 km. A job of 8 kg failed at requirement 3, a base that stops to 15 cm at 2, a camera of 12 mm at 4, a route of 30000 m at 5, a person 0.45 m from the route at 7, and a fast route with a load at 6.

#ifndef RC_RC_MOBILE_MM_JOB_HPP
#define RC_RC_MOBILE_MM_JOB_HPP

#include <cmath>
#include <rc/mobile/base_reach.hpp>
#include <rc/mobile/grasp_budget.hpp>
#include <rc/mobile/mm_supervisor.hpp>
#include <rc/mobile/parking.hpp>
#include <rc/mobile/payload_effect.hpp>
#include <rc/mobile/tip_margin.hpp>
#include <vector>

namespace rc {
namespace mobile {

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


}  // namespace mobile
}  // namespace rc

#endif
