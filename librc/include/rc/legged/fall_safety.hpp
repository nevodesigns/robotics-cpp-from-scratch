// rc/legged/fall_safety.hpp
//
// Falling with the least harm, from lesson 35-09.
//
// time_to_ground steps the inverted pendulum theta'' = (g / h) sin(theta) from a tilt and a tilt rate until the tilt reaches a right angle, and gives the time it took (capped at a limit if it never gets there, as when the body is righting itself). fall_action chooses what to do: nothing while the tilt is small and the pendulum is not running away; try to recover if there is more time to the ground than the reaction and the step need; go soft if there is less; and report that the robot has fallen once the tilt is beyond a large angle. A tilt under a warning angle that is turning slowly is left to the balance control (35-05): an inverted pendulum with any tilt at all falls in the end, so the time to the ground means something only once the tilt or its rate is large enough that the control is not holding it. impact_force is the force of a landing: the energy of the drop (mass times g times the drop) taken up over a stroke, so a stiff robot with a short stroke is hit harder than a soft one.
//
// A body 0.27 m up tilted 23 degrees and turning at 3.0 rad/s took 0.22 s to reach the ground; one tilted 17 degrees turning at 0.2 rad/s took 0.38 s. With 0.30 s needed to take a recovery step the first was told to 2 and the second to 1 (0 nothing, 1 recover, 2 go soft, 3 fallen). A 12 kg robot dropping 0.19 m was hit with 1118 N over a stroke of 0.02 m and with 224 N over 0.10 m, which is 5 times less.

#ifndef RC_RC_LEGGED_FALL_SAFETY_HPP
#define RC_RC_LEGGED_FALL_SAFETY_HPP

#include <cmath>

namespace rc {
namespace legged {

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
  const double a = std::fabs(theta);
  if (a >= fallen_angle) return kFallen;
  const double t = time_to_ground(theta, omega, h, 5.0);
  if (a < warn && std::fabs(omega) < warn_rate) return kFallNone;
  return t > t_need ? kFallRecover : kFallSoft;
}

// The force of a landing: the energy of a drop (mass * g * drop) taken up over a stroke.
inline double impact_force(double mass, double drop, double stroke) { return mass * 9.81 * drop / stroke; }


}  // namespace legged
}  // namespace rc

#endif
