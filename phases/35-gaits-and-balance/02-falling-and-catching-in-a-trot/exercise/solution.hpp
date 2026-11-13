#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// The time constant of a body whose centre of mass is h above the ground, falling about its feet.
inline double pendulum_time(double h) { return std::sqrt(h / 9.81); }

// The time for the centre of mass, x0 from the line of the feet and moving away from it at v0, to get L away; negative if it never does.
inline double time_to_move(double x0, double v0, double L, double h) {
  (void)x0;  // TODO: with T = sqrt(h / g), a = (x0 + v0 T) / 2 and b = (x0 - v0 T) / 2, x(t) = L is a u squared - L u + b = 0 for u = e^(t / T); take the root (L + sqrt(L squared - 4 a b)) / (2 a) and the time is T times its logarithm; zero if x0 is already at L, negative if there is no such time
  (void)v0;
  (void)L;
  (void)h;
  return -1.0;
}

// Whether a stance of t_stance seconds is shorter than the time to fall L by a margin factor (for example 0.7).
inline bool trot_stance_ok(double t_stance, double x0, double v0, double L, double h, double margin) {
  const double t = time_to_move(x0, v0, L, h);
  return t >= 0.0 && t_stance <= margin * t;
}


#endif
