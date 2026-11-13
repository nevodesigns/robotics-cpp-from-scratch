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
  (void)tau_meas;  // TODO: residual = measured - model; filt = alpha * residual + (1 - alpha) * filt; count the ticks in a row with |filt| over the threshold (reset to 0 otherwise); hit when the count reaches need
  (void)tau_model;
  (void)d;
  return false;
}


#endif
