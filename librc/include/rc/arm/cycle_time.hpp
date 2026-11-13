// rc/arm/cycle_time.hpp
//
// How many parts an hour, from lesson 31-07 (it extends the cycle time of 21-03).
//
// CycleParams describes one pick and place: the short vertical moves (d_app) at the pick and the place, the long carry (d_transit), the tool's speed and acceleration limits, the slower speed used on the way down onto a contact (31-04), the time for the hand to close and open (31-01) and settle, and an optional cap on the tool's speed (from a person's nearness, 31-06). cycle_time gives the seconds of one cycle at a speed override k (1 is the rated speed): each move is a trapezoid (30-06) with the speed k v capped, and the acceleration k squared a; the two moves down onto a contact are held to the contact speed; the hand's time is added unscaled. scale_for_cycle finds the override that gives a target cycle time (bisection, since the contact speed does not scale), or minus one if even the rated speed is too slow. throughput is cycles an hour.
//
// A cycle at the rated speed took 3.68 s (977 an hour); at half speed 5.70 s and at a quarter 10.13 s. The hand took 0.8 s of the 3.68 and the two slow approaches 1.07 s. A target of 5.16 s needed an override of 0.576; a target of 2.95 s could not be reached (it gave -1). A person 1.0 m from the robot cut the tool speed to 1.23 m/s and the cycle to 3.77 s, one at 2.0 m to 2.49 m/s and 3.68 s.

#ifndef RC_RC_ARM_CYCLE_TIME_HPP
#define RC_RC_ARM_CYCLE_TIME_HPP

#include <cmath>
#include <rc/arm/gripper.hpp>
#include <rc/arm/ssm.hpp>
#include <rc/arm/time_param.hpp>

namespace rc {
namespace arm {

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
  if (cycle_time(p, 1.0) > target) return -1.0;
  double lo = 1e-3, hi = 1.0;
  for (int i = 0; i < 80; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (cycle_time(p, mid) > target) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return hi;
}

// The cycle with a person at `separation` metres: the tool's speed is capped by the separation lesson's limit.
inline double cycle_with_person(CycleParams p, const SsmParams& ssm, double separation) {
  p.v_cap = max_speed(ssm, separation);
  if (p.v_cap <= 0.0) return -1.0;  // the robot must stop
  return cycle_time(p, 1.0);
}


}  // namespace arm
}  // namespace rc

#endif
