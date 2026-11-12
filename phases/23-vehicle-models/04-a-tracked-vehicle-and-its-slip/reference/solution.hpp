#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <rc/vehicle/diff_slip.hpp>

using namespace rc::vehicle;

// The soil under a track: how much it holds together (c, Pa), how much it grips under load (phi, radians) and how far it shears before it gives its full strength (K, metres).
struct Soil {
  double cohesion = 1000.0;
  double friction_angle = 0.4887;  // 28 degrees
  double shear_modulus = 0.025;
};

// One track: the length in contact, its width, and the share of the weight it carries (N).
struct Track {
  double length = 0.40;
  double width = 0.08;
  double load = 98.1;
};

// The largest thrust the soil can give a track: its cohesion over the area in contact, and the friction under its load.
inline double max_thrust(const Soil& s, const Track& t) { return s.cohesion * t.length * t.width + t.load * std::tan(s.friction_angle); }

// The thrust of a track at a slip i (0 to 1): none with no slip, and the largest as the slip grows.
inline double track_thrust(const Soil& s, const Track& t, double slip) {
  if (slip <= 1e-12) return 0.0;
  const double x = slip * t.length / s.shear_modulus;
  return max_thrust(s, t) * (1.0 - (1.0 - std::exp(-x)) / x);
}

// The slip at which a track gives a thrust, by bisection; 1 if the soil cannot give it.
inline double slip_for_thrust(const Soil& s, const Track& t, double thrust) {
  if (thrust >= max_thrust(s, t)) return 1.0;
  double lo = 0.0;
  double hi = 1.0;
  for (int i = 0; i < 60; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (track_thrust(s, t, mid) < thrust) lo = mid;
    else hi = mid;
  }
  return 0.5 * (lo + hi);
}

// The thrust each track must give to turn the vehicle on the spot against the friction of the ground, when the pressure is even along the tracks: the resistance is mu m g L / 4, and the two tracks give a torque of thrust times the gauge.
inline double pivot_thrust_tracked(double mass, double mu, double track_length, double gauge) { return mu * mass * 9.81 * track_length / (4.0 * gauge); }

// A differential drive whose two tracks slip by their own amounts.
class TrackedDrive {
 public:
  TrackedDrive(double sprocket_radius, double gauge, double chi) : r_(sprocket_radius), b_(gauge), chi_(chi) {}
  void reset(const Pose2& p) { pose_ = p; }
  const Pose2& pose() const { return pose_; }

  // Track speeds in rad/s of the drive sprockets, and the slip of each track.
  void step(double wl, double wr, double slip_l, double slip_r, double dt) {
    const double vl = r_ * wl * (1.0 - slip_l);
    const double vr = r_ * wr * (1.0 - slip_r);
    const double v = 0.5 * (vl + vr);
    const double w = (vr - vl) / (chi_ * b_);
    const double mid = pose_.theta + 0.5 * w * dt;
    pose_.x += v * std::cos(mid) * dt;
    pose_.y += v * std::sin(mid) * dt;
    pose_.theta += w * dt;
  }

 private:
  double r_;
  double b_;
  double chi_;
  Pose2 pose_;
};


#endif
