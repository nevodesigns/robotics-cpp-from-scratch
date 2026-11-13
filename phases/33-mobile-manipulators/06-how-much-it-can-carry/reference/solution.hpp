#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/arm/limits_heat.hpp>
#include <rc/mobile/mm_frames.hpp>
#include <rc/mobile/tip_margin.hpp>
#include <vector>

using namespace rc::arm;
using namespace rc::mobile;

using rc::arm::ArmModel;
using rc::arm::max_payload;

struct BaseDrive {
  double wheel_force = 80.0;   // N, all the wheels together (typical)
  double mu = 0.6;             // the floor (typical)
  double c_rr = 0.02;          // rolling resistance (typical, small wheels on a hard floor)
  double battery_wh = 100.0;   // typical
  double efficiency = 0.7;     // typical
  double idle_w = 15.0;        // W, the electronics (typical)
};

struct Requirements {
  double stop_speed = 1.5;     // m/s
  double stop_dist = 0.20;     // m, an emergency stop
  double accel_wanted = 0.5;   // m/s2
  double half_wheelbase = 0.30;
  double half_track = 0.22;
};

struct Budget {
  double arm = 0.0;        // kg
  double tip = 0.0;
  double traction = 0.0;
  double best = 0.0;
  int binding = 0;         // 0 the arm, 1 tipping, 2 traction
};

inline double base_accel_limit(const BaseDrive& d, double mass) {
  const double by_force = d.wheel_force / mass;
  const double by_grip = d.mu * rc::safety::kGravity;
  return by_force < by_grip ? by_force : by_grip;
}

// How far a battery takes the base at speed v (m/s) with a total mass (kg).
inline double range_m(const BaseDrive& d, double mass, double v) {
  const double power = d.c_rr * mass * rc::safety::kGravity * v / d.efficiency + d.idle_w;
  return d.battery_wh * 3600.0 / power * v;
}

// The three limits on the payload and the least of them.
inline Budget payload_budget(const MobileManipulator& mm, const BasePose& b, const std::vector<double>& q, const MassModel& masses, const ArmModel& model, const std::vector<double>& joint_limits, double rated, const BaseDrive& drive, const Requirements& req) {
  Budget out;
  out.arm = max_payload(model, q, mm.tool, joint_limits, rated);
  const double a_req = req.stop_speed * req.stop_speed / (2.0 * req.stop_dist);
  double lo = 0.0, hi = 50.0;
  MassModel mt = masses;
  double com[3];
  mt.payload = 0.0;
  com_world(mm, b, q, mt, com);
  if (tip_limits(b, com, req.half_wheelbase, req.half_track).brake < a_req) {
    out.tip = -1.0;
  } else {
    mt.payload = hi;
    com_world(mm, b, q, mt, com);
    if (tip_limits(b, com, req.half_wheelbase, req.half_track).brake >= a_req) {
      out.tip = hi;
    } else {
      for (int i = 0; i < 60; ++i) {
        const double mid = 0.5 * (lo + hi);
        mt.payload = mid;
        com_world(mm, b, q, mt, com);
        if (tip_limits(b, com, req.half_wheelbase, req.half_track).brake >= a_req) {
          lo = mid;
        } else {
          hi = mid;
        }
      }
      out.tip = lo;
    }
  }
  double fixed = masses.base_mass;
  for (std::size_t i = 0; i < masses.link_mass.size(); ++i) fixed += masses.link_mass[i];
  out.traction = drive.mu * rc::safety::kGravity >= req.accel_wanted ? drive.wheel_force / req.accel_wanted - fixed : -1.0;
  out.best = out.arm;
  out.binding = 0;
  if (out.tip < out.best) {
    out.best = out.tip;
    out.binding = 1;
  }
  if (out.traction < out.best) {
    out.best = out.traction;
    out.binding = 2;
  }
  return out;
}


#endif
