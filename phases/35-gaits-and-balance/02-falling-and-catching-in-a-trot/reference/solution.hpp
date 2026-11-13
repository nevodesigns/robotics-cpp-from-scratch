#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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


#endif
