// rc/vehicle/skid_steer.hpp
//
// A skid steer vehicle, from lesson 23-02.
//
// A vehicle with fixed wheels on each side turns only by sliding them sideways. Its motion is the motion of a differential drive whose track is widened by the ICR coefficient chi (one for ideal rolling, more on a floor that grips): SkidSteer is a DiffSlip with a turn efficiency of 1 / chi. skid_motion gives the speed, the turn rate and the radius of the instantaneous centre of rotation. front_slip_angle is the angle at which the front wheels slide in a pivot, atan(L / B), whatever the speed. pivot_thrust_per_side is the force each side must give to turn the vehicle on the spot against the scrubbing of its four wheels, mu m g sqrt(L2 + B2) / (2 B).
//
// A chassis 0.30 m long and 0.25 m wide slid its wheels at 50.2 degrees in a pivot; chi of 1.4 and an efficiency of 1 / 1.4 gave the same path; a centre of rotation at 0.625 m at chi 1 was at 0.938 m at chi 1.5; a 10 kg vehicle needed 34.5 N a side to pivot on gravel and 7.7 N on ice against 28.0 N from two gear motors a side, so it cannot pivot on gravel.

#ifndef RC_RC_VEHICLE_SKID_STEER_HPP
#define RC_RC_VEHICLE_SKID_STEER_HPP

#include <cmath>
#include <rc/vehicle/diff_slip.hpp>

namespace rc {
namespace vehicle {

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


}  // namespace vehicle
}  // namespace rc

#endif
