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
  (void)b;  // TODO: tip: the heaviest payload (halve an interval from 0 to 50 kg) for which tip_limits(...).brake, with com_world including that payload, is at least the deceleration an emergency stop needs, v squared over twice the distance (negative if even no payload is enough); traction: the wheel force over the wanted acceleration less the fixed mass of base and arm (negative if friction cannot give that acceleration); best is the least of the three and binding says which
  (void)masses;
  (void)drive;
  (void)req;
  out.best = out.arm;
  return out;
}


#endif
