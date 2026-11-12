#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/vehicle/diff_slip.hpp>

using namespace rc::vehicle;

// The geometry and weight of a four wheel vehicle with fixed wheels.
struct SkidGeometry {
  double wheelbase = 0.30;  // L: front axle to rear axle, metres
  double track = 0.25;      // B: left wheels to right wheels, metres
  double wheel_radius = 0.05;
  double mass = 10.0;       // kg
};

// A skid steer vehicle: the motion of a differential drive, with the turn made smaller by the ICR coefficient chi (1 or more).
class SkidSteer {
 public:
  SkidSteer(const SkidGeometry& g, double chi, double slip) : g_(g), chi_(chi), model_(g.wheel_radius, g.track, slip, 1.0 / chi) {}
  void step(double wl, double wr, double dt) { model_.step(wl, wr, dt); }
  const Pose2& pose() const { return model_.pose(); }
  double chi() const { return chi_; }
  const SkidGeometry& geometry() const { return g_; }

 private:
  SkidGeometry g_;
  double chi_;
  DiffSlip model_;
};

struct SkidMotion {
  double v = 0.0;           // speed of the centre, m/s
  double w = 0.0;           // turn rate, rad/s
  double icr_radius = 0.0;  // v / w, metres; infinite for a straight line, zero for a pivot
};

inline SkidMotion skid_motion(const SkidGeometry& g, double chi, double wl, double wr) {
  SkidMotion m;
  m.v = 0.5 * g.wheel_radius * (wl + wr);
  m.w = g.wheel_radius * (wr - wl) / (chi * g.track);
  m.icr_radius = std::fabs(m.w) > 1e-12 ? m.v / m.w : 1.0e12;
  return m;
}

// In a pivot the wheel at (x, y) from the centre moves sideways at omega * x and along the vehicle at omega * y, but can only roll along it: the sideways part is a slide. At the front wheels, the angle of the slide to the way they roll.
inline double front_slip_angle(double wheelbase, double track) { return std::atan2(wheelbase, track); }

// The force each side must give to turn on the spot: the four wheels each carry a quarter of the weight and scrub against a friction mu, at a distance of half the diagonal from the centre; the two sides give a torque of force times the track.
inline double pivot_thrust_per_side(double mass, double mu, double wheelbase, double track) {
  const double g = 9.81;
  return mu * mass * g * std::sqrt(wheelbase * wheelbase + track * track) / (2.0 * track);
}

inline bool can_pivot(double thrust_per_side, double mass, double mu, double wheelbase, double track) {
  return thrust_per_side >= pivot_thrust_per_side(mass, mu, wheelbase, track);
}


#endif
