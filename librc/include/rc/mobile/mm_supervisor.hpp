// rc/mobile/mm_supervisor.hpp
//
// One speed budget for base and arm, from lesson 33-09.
//
// tip_speed_limit is the fastest the base may go and still stop within a distance by braking at a fraction (the margin) of the deceleration that would tip it (33-05): the square root of twice the margin, the tipping deceleration and the distance. share_budget takes the speeds asked of the base and of the hand toward a person and the total that the person's nearness allows (31-06): the base is first held to its own limit, and if the two together are still over the total both are scaled by the same factor, so that neither is starved. supervise gives the limited speeds, a status (0 free, 1 scaled, 2 stopped) and which limit was binding (0 none, 1 the load's, 2 the person's).
//
// With the load the base could brake at 5.19 m/s2 without tipping, so with a margin of 0.8 and 0.5 m to stop in it was held to 2.04 m/s. A person 4.0 m away left the speeds as asked (0), one at 1.0 m allowed 1.23 m/s in all and the base's 1.0 m/s and the hand's 0.5 m/s were scaled to 0.82 and 0.41, and one at 0.2 m (inside the distance for standing still) stopped both (2). In a simulation of a person walking at the robot at 1.6 m/s the supervised robot came to rest 0.14 m away (28 mm inside the 0.17 m of margins), and an unsupervised one at the same speeds met the person, closing on them at 3.1 m/s.

#ifndef RC_RC_MOBILE_MM_SUPERVISOR_HPP
#define RC_RC_MOBILE_MM_SUPERVISOR_HPP

#include <cmath>
#include <rc/arm/ssm.hpp>

namespace rc {
namespace mobile {

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
  double b = base_ask < base_cap ? base_ask : base_cap;
  double h = hand_ask;
  if (b < base_ask) out.binding = 1;
  if (total <= 1e-12) {
    out.base = 0.0;
    out.hand = 0.0;
    out.status = 2;
    out.binding = 2;
    return out;
  }
  const double sum = (b > 0.0 ? b : 0.0) + (h > 0.0 ? h : 0.0);
  if (sum > total) {
    const double f = total / sum;
    b = b > 0.0 ? b * f : b;
    h = h > 0.0 ? h * f : h;
    out.status = 1;
    out.binding = 2;
  } else if (b < base_ask) {
    out.status = 1;
  }
  out.base = b;
  out.hand = h;
  return out;
}

// The supervisor: the person at `separation`, the speeds asked, the load's tipping deceleration and the distance to stop in.
inline SpeedLimits supervise(const SsmParams& ssm, double separation, double base_ask, double hand_ask, double tip_brake, double margin, double stop_dist) {
  return share_budget(base_ask, hand_ask, tip_speed_limit(tip_brake, margin, stop_dist), max_speed(ssm, separation));
}


}  // namespace mobile
}  // namespace rc

#endif
