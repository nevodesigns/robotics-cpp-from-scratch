#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/arm/gripper.hpp>
#include <rc/arm/ssm.hpp>
#include <rc/arm/time_param.hpp>

using namespace rc::arm;

struct CycleParams {
  double d_app = 0.10;       // m, the vertical move at the pick and at the place
  double d_transit = 0.60;   // m, the carry (and the return)
  double v = 1.5;            // m/s, the tool's rated speed (typical)
  double a = 6.0;            // m/s2, the tool's rated acceleration (typical)
  double v_contact = 0.2;    // m/s, the speed for going down onto a contact (31-04)
  double close_s = 0.3;      // s, the hand closing (31-01)
  double open_s = 0.3;       // s, the hand opening
  double settle_s = 0.1;     // s, after each of closing and opening
  double v_cap = 1e9;        // m/s, a cap on the tool's speed (31-06)
};

// Seconds of one pick and place at the override k (1 is the rated speed).
inline double cycle_time(const CycleParams& p, double k) {
  const double vk = std::fmin(k * p.v, p.v_cap);
  const double ak = k * k * p.a;
  const double vc = std::fmin(vk, p.v_contact);
  // The moves: down to the pick, lift, carry, down to the place, lift, and the return.
  const double moves = trapezoid_time(p.d_app, vc, ak) + trapezoid_time(p.d_app, vk, ak) + trapezoid_time(p.d_transit, vk, ak) + trapezoid_time(p.d_app, vc, ak) + trapezoid_time(p.d_app, vk, ak) + trapezoid_time(p.d_transit, vk, ak);
  return moves + p.close_s + p.settle_s + p.open_s + p.settle_s;
}

inline double throughput(double cycle_seconds) { return cycle_seconds > 0.0 ? 3600.0 / cycle_seconds : 0.0; }

// The override in (0, 1] that gives a cycle of `target` seconds; -1 if even the rated speed is too slow.
inline double scale_for_cycle(const CycleParams& p, double target) {
  (void)p;  // TODO: the cycle time falls as k rises; if k = 1 is not fast enough return -1, else bisect k between a small number and 1 until cycle_time meets the target
  (void)target;
  return 1.0;
}

// The cycle with a person at `separation` metres: the tool's speed is capped by the separation lesson's limit.
inline double cycle_with_person(CycleParams p, const SsmParams& ssm, double separation) {
  p.v_cap = max_speed(ssm, separation);
  if (p.v_cap <= 0.0) return -1.0;  // the robot must stop
  return cycle_time(p, 1.0);
}


#endif
