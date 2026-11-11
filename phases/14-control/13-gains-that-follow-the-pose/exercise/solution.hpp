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
inline double lookup(const std::vector<double>& xs, const std::vector<double>& ys, double x) {
  // TODO: `xs` rises. n is the shorter of the two lengths; with n of 0 the answer is 0.
  // At or below xs[0] give ys[0], at or above xs[n - 1] give ys[n - 1].
  // In between find the segment x falls in and join its ends with a straight line.
  (void)xs;
  (void)ys;
  (void)x;
  return 0.0;
}

// The gains that put a joint of this inertia at natural frequency `wn` and damping ratio `zeta`.
inline Gains scheduled_gains(double inertia, double wn, double zeta) {
  // TODO: kp = inertia * wn * wn, kd = 2 * zeta * inertia * wn. Zero gains for an inertia or wn of zero or less.
  (void)inertia;
  (void)wn;
  (void)zeta;
  return Gains{};
}

// A step to 1 on a joint of this inertia with a little damping, 0.1, under a PD controller, 5 s in steps of 1 ms.
inline JointResult run_joint(double inertia, double kp, double kd) {
  // TODO: 5000 steps of dt = 1e-3, starting at rest at position 0.
  // Each step: torque = kp * (1 - position) - kd * velocity;
  //            velocity += (torque - 0.1 * velocity) / inertia * dt; position += velocity * dt.
  // Track the peak position, and the last step on which |1 - position| was over 0.02.
  // overshoot = peak - 1 (never below 0). settling_time = (that last step + 1) * dt, unless it was
  // the final step, when it never stayed in the band and the time stays at -1.
  (void)inertia;
  (void)kp;
  (void)kd;
  return JointResult{};
}

// Gains for any extension, from a table made at a few extensions.
class GainSchedule {
 public:
  // One knot at each of `extensions`, with gains designed for the inertia there.
  GainSchedule(const std::vector<double>& extensions, double wn, double zeta) : extensions_(extensions) {
    // TODO: for each extension, scheduled_gains(arm_inertia(extension), wn, zeta), and push its kp onto
    // kps_ and its kd onto kds_.
    (void)wn;
    (void)zeta;
  }

  Gains at(double extension) const {
    // TODO: kp and kd from lookup() on the extensions and the two gain tables.
    (void)extension;
    (void)kps_;
    (void)kds_;
    return Gains{};
  }

  std::size_t knots() const { return extensions_.size(); }

 private:
  std::vector<double> extensions_;
  std::vector<double> kps_;
  std::vector<double> kds_;
};

#endif  // LESSON_SOLUTION_HPP
