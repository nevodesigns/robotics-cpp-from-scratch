// rc/arm/contact.hpp
//
// Feeling that it has touched, from lesson 31-04.
//
// ContactDetector looks at the residual of one joint: the torque measured (from the motor current, in N m) less the torque the model says the free motion would need (gravity, friction and inertia). The residual is smoothed by a first order filter (alpha is the share of the new), and contact is declared when its size has been over the threshold for `need` ticks in a row. update takes the two torques, returns whether contact is declared, and keeps the state.
//
// A joint driven at 0.2 rad/s into a wall was found to touch 15 ms after it reached it, with the wall pushing back with 1.31 N m by then; at 0.6 rad/s the same detector took 10 ms and the wall's torque reached 5.20 N m. With a gravity term that was 5 percent out, a threshold of 0.05 N m gave 1 false alarms before the wall was reached and one of 0.8 N m gave 0.

#ifndef RC_RC_ARM_CONTACT_HPP
#define RC_RC_ARM_CONTACT_HPP

#include <cmath>

namespace rc {
namespace arm {

struct ContactDetector {
  double thr = 0.5;     // N m, the residual that counts
  int need = 5;         // ticks it must stay over
  double alpha = 0.2;   // the share of the new residual in the filter
  double filt = 0.0;
  int count = 0;
  bool hit = false;
};

// One tick: the measured joint torque and the torque the model says free motion needs; whether contact is declared.
inline bool contact_update(ContactDetector& d, double tau_meas, double tau_model) {
  const double r = tau_meas - tau_model;
  d.filt = d.alpha * r + (1.0 - d.alpha) * d.filt;
  if (std::fabs(d.filt) > d.thr) {
    ++d.count;
  } else {
    d.count = 0;
  }
  if (d.count >= d.need) d.hit = true;
  return d.hit;
}


}  // namespace arm
}  // namespace rc

#endif
