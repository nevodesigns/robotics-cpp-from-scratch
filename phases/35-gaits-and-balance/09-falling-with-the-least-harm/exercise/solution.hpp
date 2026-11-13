#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

enum FallAction { kFallNone = 0, kFallRecover = 1, kFallSoft = 2, kFallen = 3 };

// The time for a body h up, tilted theta0 and turning at omega0 (rad, rad/s), to fall to a right angle, by stepping theta'' = (g / h) sin(theta); `cap` if it never gets there.
inline double time_to_ground(double theta0, double omega0, double h, double cap) {
  const double dt = 0.0005, k = 9.81 / h, target = 1.5707963267948966;
  double th = theta0, om = omega0, t = 0.0;
  const double sign = th >= 0.0 && !(th == 0.0 && om < 0.0) ? 1.0 : -1.0;
  th *= sign;
  om *= sign;
  while (t < cap) {
    if (th >= target) return t;
    om += k * std::sin(th) * dt;
    th += om * dt;
    t += dt;
  }
  return cap;
}

// What to do: nothing while the tilt and its rate are small (the balance control of 35-05 holds it), try to recover if there is time, go soft if there is not, and fallen beyond a large tilt.
inline int fall_action(double theta, double omega, double h, double t_need, double warn, double warn_rate, double fallen_angle) {
  (void)theta;  // TODO: fallen if the tilt is past fallen_angle; nothing if the tilt is under `warn` and the rate under `warn_rate`; else recover if the time to the ground is more than t_need and soft if not
  (void)omega;
  (void)h;
  (void)t_need;
  (void)warn;
  (void)warn_rate;
  (void)fallen_angle;
  return kFallNone;
}

// The force of a landing: the energy of a drop (mass * g * drop) taken up over a stroke.
inline double impact_force(double mass, double drop, double stroke) { return mass * 9.81 * drop / stroke; }


#endif
