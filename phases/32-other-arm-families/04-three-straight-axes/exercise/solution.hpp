#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/arm/time_param.hpp>

using namespace rc::arm;

struct Gantry {
  double v[3] = {3.0, 3.0, 0.5};     // m/s for x, y, z (typical: belts on the long axes, a screw on z)
  double a[3] = {5.0, 5.0, 20.0};    // m/s2 (typical: belts give speed, a screw gives acceleration)
};

// The time when each axis goes on its own: the move is over when the slowest is.
inline double axes_time(const Gantry& g, const double d[3]) {
  double t = 0.0;
  for (int i = 0; i < 3; ++i) {
    const double ti = trapezoid_time(std::fabs(d[i]), g.v[i], g.a[i]);
    if (ti > t) t = ti;
  }
  return t;
}

// The time when the tool goes in a straight line to d.
inline double straight_line_time(const Gantry& g, const double d[3]) {
  (void)g;  // TODO: the length of d; for each axis with a share u = |d_i| / length of the direction the path speed is at most v_i / u and the acceleration at most a_i / u; take the smallest of each and use trapezoid_time of the length
  (void)d;
  return 0.0;
}

// The deflection of a beam of length `len`, stiffness E I (N m2), supported at its ends and loaded by f at its middle.
inline double beam_sag(double f, double len, double ei) { return f * len * len * len / (48.0 * ei); }

inline double beam_stiffness(double len, double ei) { return 48.0 * ei / (len * len * len); }

inline double first_mode_hz(double k, double mass) { return std::sqrt(k / mass) / (2.0 * 3.14159265358979323846); }

// The sideways error at `lever` metres from a beam driven from both ends, the ends being out of step by `skew` metres over `span`.
inline double rack_error(double skew, double span, double lever) { return skew / span * lever; }


#endif
