// rc/mobile/payload_effect.hpp
//
// How much it can carry, from lesson 33-06.
//
// BaseDrive describes the base's drive: the force its wheels can give in all (N), the friction of the floor, the rolling resistance, the battery's energy, the efficiency of the drive and what the electronics use. Requirements are what the base must still be able to do with the load: stop from a speed in a distance (an emergency stop), and accelerate at a wanted rate. base_accel_limit is the acceleration the wheels can give a mass, the least of force over mass and the friction times g. range_m is how far a battery takes the base at a speed with a mass. payload_budget gives the three limits on the payload and the least of them: the arm's (the heaviest the joints hold at the pose, from 29-08), the tipping one (the heaviest for which the base can still brake at the deceleration an emergency stop needs, found by halving an interval, from 33-05) and the traction one (the heaviest for which the wheels can still give the wanted acceleration), with which one was binding.
//
// With the arm reaching 0.65 m the joints held 5.0 kg; the base could carry 4.3 kg and still stop from 1.5 m/s in 0.20 m without tipping, which needs 5.62 m/s2, and 130.7 kg and still accelerate at 0.5 m/s2. The least was 4.3 kg and the binding limit was number 1 (0 the arm, 1 tipping, 2 traction). With 10 kg aboard the range on the battery went from 15509 m to 13838 m.

#ifndef RC_RC_MOBILE_PAYLOAD_EFFECT_HPP
#define RC_RC_MOBILE_PAYLOAD_EFFECT_HPP

#include <cmath>
#include <rc/arm/limits_heat.hpp>
#include <rc/mobile/mm_frames.hpp>
#include <rc/mobile/tip_margin.hpp>
#include <vector>

namespace rc {
namespace mobile {

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


}  // namespace mobile
}  // namespace rc

#endif
