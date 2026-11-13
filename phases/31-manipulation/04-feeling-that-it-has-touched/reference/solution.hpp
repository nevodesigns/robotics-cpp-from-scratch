#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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


#endif
