#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/arm/ssm.hpp>

using namespace rc::arm;

using rc::arm::max_speed;
using rc::arm::SsmParams;

struct SpeedLimits {
  double base = 0.0;     // m/s toward the person
  double hand = 0.0;     // m/s toward the person
  int status = 0;        // 0 free, 1 scaled, 2 stopped
  int binding = 0;       // 0 none, 1 the load's limit, 2 the person's
};

// The fastest the base may go and still stop in `dist` metres by braking at `margin` times the tipping deceleration.
inline double tip_speed_limit(double tip_brake, double margin, double dist) { return std::sqrt(2.0 * margin * tip_brake * dist); }

// The speeds toward a person, limited: the base to its own limit, then base and hand together to the total, both scaled by one factor.
inline SpeedLimits share_budget(double base_ask, double hand_ask, double base_cap, double total) {
  SpeedLimits out;
  (void)base_ask;  // TODO: hold the base to base_cap; if total is zero stop both (status 2, binding 2); if the base and hand speeds toward the person together are over total, scale both by total over their sum (status 1, binding 2); status 1 and binding 1 if only the base cap acted
  (void)hand_ask;
  (void)base_cap;
  (void)total;
  out.base = base_ask;
  out.hand = hand_ask;
  return out;
}

// The supervisor: the person at `separation`, the speeds asked, the load's tipping deceleration and the distance to stop in.
inline SpeedLimits supervise(const SsmParams& ssm, double separation, double base_ask, double hand_ask, double tip_brake, double margin, double stop_dist) {
  return share_budget(base_ask, hand_ask, tip_speed_limit(tip_brake, margin, stop_dist), max_speed(ssm, separation));
}


#endif
