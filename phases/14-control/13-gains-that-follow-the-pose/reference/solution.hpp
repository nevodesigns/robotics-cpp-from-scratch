#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

// A proportional and a derivative gain.
struct Gains {
  double kp = 0.0;
  double kd = 0.0;
};

// What a step response did.
struct JointResult {
  double overshoot = 0.0;        // past the target, as a fraction of it
  double settling_time = -1.0;   // when it last left a 2 percent band, negative if it never stayed in
};

// How far the arm reaches out, 0 folded to 1 fully extended, and what that does to the inertia about the joint:
// the mass is further out, and the inertia goes with the square of the distance.
inline double arm_inertia(double extension) {
  const double e = extension < 0.0 ? 0.0 : (extension > 1.0 ? 1.0 : extension);
  return 0.5 + 2.5 * e * e;
}

// The value of a table at x, joining the points with straight lines and holding the end values beyond them.
// `xs` rises. With no points the answer is zero, and with mismatched lengths only the shorter is used.
inline double lookup(const std::vector<double>& xs, const std::vector<double>& ys, double x) {
  const std::size_t n = xs.size() < ys.size() ? xs.size() : ys.size();
  if (n == 0) return 0.0;
  if (x <= xs[0]) return ys[0];
  if (x >= xs[n - 1]) return ys[n - 1];
  std::size_t i = 1;
  while (i < n - 1 && x > xs[i]) ++i;
  const double span = xs[i] - xs[i - 1];
  if (span <= 0.0) return ys[i];
  return ys[i - 1] + (ys[i] - ys[i - 1]) * (x - xs[i - 1]) / span;
}

// The gains that put a joint of this inertia at natural frequency `wn` and damping ratio `zeta`.
inline Gains scheduled_gains(double inertia, double wn, double zeta) {
  Gains g;
  if (inertia <= 0.0 || wn <= 0.0) return g;
  g.kp = inertia * wn * wn;
  g.kd = 2.0 * zeta * inertia * wn;
  return g;
}

// A step to 1 on a joint of this inertia with a little damping, 0.1, under a PD controller, 5 s in steps of 1 ms.
inline JointResult run_joint(double inertia, double kp, double kd) {
  const double dt = 1e-3;
  const int steps = 5000;
  double position = 0.0;
  double velocity = 0.0;
  double peak = 0.0;
  long last_outside = -1;
  for (int i = 0; i < steps; ++i) {
    const double torque = kp * (1.0 - position) - kd * velocity;
    velocity += (torque - 0.1 * velocity) / inertia * dt;
    position += velocity * dt;
    if (position > peak) peak = position;
    if (std::fabs(1.0 - position) > 0.02) last_outside = i;
  }
  JointResult r;
  r.overshoot = peak > 1.0 ? peak - 1.0 : 0.0;
  if (last_outside + 1 < steps) r.settling_time = static_cast<double>(last_outside + 1) * dt;
  return r;
}

// Gains for any extension, from a table made at a few extensions.
class GainSchedule {
 public:
  // One knot at each of `extensions`, with gains designed for the inertia there.
  GainSchedule(const std::vector<double>& extensions, double wn, double zeta) : extensions_(extensions) {
    for (double e : extensions_) {
      const Gains g = scheduled_gains(arm_inertia(e), wn, zeta);
      kps_.push_back(g.kp);
      kds_.push_back(g.kd);
    }
  }

  Gains at(double extension) const {
    Gains g;
    g.kp = lookup(extensions_, kps_, extension);
    g.kd = lookup(extensions_, kds_, extension);
    return g;
  }

  std::size_t knots() const { return extensions_.size(); }

 private:
  std::vector<double> extensions_;
  std::vector<double> kps_;
  std::vector<double> kds_;
};

#endif  // LESSON_SOLUTION_HPP
