// rc/legged/trot_balance.hpp
//
// Falling and catching in a trot, from lesson 35-02.
//
// pendulum_time is the time constant T = sqrt(h/g) of a body whose centre of mass is a height h above the ground, falling about the line of its two feet: the sideways distance x of the centre from the line obeys x'' = (g/h) x for small angles, so x(t) = x0 cosh(t/T) + v0 T sinh(t/T). time_to_move gives the time it takes the centre to move out to a distance L from the line, starting at x0 with speed v0 away from it, in closed form (with u = e to the t over T the condition is a quadratic in u); it is negative if the centre never gets there (it is moving back toward the line fast enough to stop first). trot_stance_ok says whether a stance of a given length is shorter than that time by a margin, which is whether the swinging feet will have landed before the body has fallen too far.
//
// For a body 0.27 m high the time constant was 0.166 s. A centre of mass 10 mm off the line of the feet took 0.41 s to fall 6 cm, one 30 mm off took 0.22 s, and one 30 mm off and already moving away at 0.10 m/s took 0.15 s. A stance of 0.25 s was therefore 1 for the first (1 means short enough with the margin) and 0 for the second. A body 0.40 m high, the same offset, took 0.50 s.

#ifndef RC_RC_LEGGED_TROT_BALANCE_HPP
#define RC_RC_LEGGED_TROT_BALANCE_HPP

#include <cmath>

namespace rc {
namespace legged {

// The time constant of a body whose centre of mass is h above the ground, falling about its feet.
inline double pendulum_time(double h) { return std::sqrt(h / 9.81); }

// The time for the centre of mass, x0 from the line of the feet and moving away from it at v0, to get L away; negative if it never does.
inline double time_to_move(double x0, double v0, double L, double h) {
  const double T = pendulum_time(h);
  const double a = 0.5 * (x0 + v0 * T), b = 0.5 * (x0 - v0 * T);
  if (x0 >= L) return 0.0;
  if (a <= 0.0) return -1.0;
  const double disc = L * L - 4.0 * a * b;
  if (disc < 0.0) return -1.0;
  const double u = (L + std::sqrt(disc)) / (2.0 * a);
  if (u <= 0.0) return -1.0;
  return T * std::log(u);
}

// Whether a stance of t_stance seconds is shorter than the time to fall L by a margin factor (for example 0.7).
inline bool trot_stance_ok(double t_stance, double x0, double v0, double L, double h, double margin) {
  const double t = time_to_move(x0, v0, L, h);
  return t >= 0.0 && t_stance <= margin * t;
}


}  // namespace legged
}  // namespace rc

#endif
