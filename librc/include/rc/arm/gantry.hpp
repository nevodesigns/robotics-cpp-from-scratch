// rc/arm/gantry.hpp
//
// Three straight axes, from lesson 32-04.
//
// Gantry holds, for each of the three axes, the speed and acceleration limits. axes_time is the time of a move when each axis goes on its own and the move is over when the slowest axis is: it is the shortest time, and the path is not straight. straight_line_time is the time when the tool is to go in a straight line: along a direction with unit vector u the path may go at no more than v_i over |u_i| for each axis and at no more than a_i over |u_i| for its acceleration, and the time is that of a trapezoid at the smaller of each. beam_sag gives the deflection of a beam of length L, stiffness E I, supported at its ends and loaded at its middle by F: F L cubed over 48 E I. beam_stiffness is the force over that sag. first_mode_hz is the resonance of a mass on a spring. rack_error is the sideways error at a lever arm from a beam that is driven from both ends and whose ends are out of step by some distance over a span.
//
// A move of 0.4 m in x and 0.4 m in z took 0.825 s with each axis on its own and 0.900 s in a straight line, because z limits the speed and x the acceleration. A beam of 1.0 m under 50 N sagged 50 micrometres, which is a stiffness of 1.0 MN/m and, with 5 kg moving, a first resonance of 71 Hz; twice the span made the sag 8 times as great. Ends 0.2 mm out of step on a span of 1.0 m put 60 micrometres of error at a lever of 0.3 m.

#ifndef RC_RC_ARM_GANTRY_HPP
#define RC_RC_ARM_GANTRY_HPP

#include <cmath>
#include <rc/arm/time_param.hpp>

namespace rc {
namespace arm {

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
  const double dist = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
  if (dist <= 0.0) return 0.0;
  double vp = 1e18, ap = 1e18;
  for (int i = 0; i < 3; ++i) {
    const double u = std::fabs(d[i]) / dist;
    if (u < 1e-12) continue;
    if (g.v[i] / u < vp) vp = g.v[i] / u;
    if (g.a[i] / u < ap) ap = g.a[i] / u;
  }
  return trapezoid_time(dist, vp, ap);
}

// The deflection of a beam of length `len`, stiffness E I (N m2), supported at its ends and loaded by f at its middle.
inline double beam_sag(double f, double len, double ei) { return f * len * len * len / (48.0 * ei); }

inline double beam_stiffness(double len, double ei) { return 48.0 * ei / (len * len * len); }

inline double first_mode_hz(double k, double mass) { return std::sqrt(k / mass) / (2.0 * 3.14159265358979323846); }

// The sideways error at `lever` metres from a beam driven from both ends, the ends being out of step by `skew` metres over `span`.
inline double rack_error(double skew, double span, double lever) { return skew / span * lever; }


}  // namespace arm
}  // namespace rc

#endif
