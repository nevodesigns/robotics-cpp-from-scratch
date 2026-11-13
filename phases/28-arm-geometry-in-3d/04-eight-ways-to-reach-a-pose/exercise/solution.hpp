#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/fk.hpp>
#include <vector>

using namespace rc::arm;

// A typical six joint arm with an elbow and a spherical wrist: standard convention, dimensions typical.
inline ArmTable elbow_arm() {
  ArmTable t;
  t.name = "typical elbow arm with a spherical wrist";
  t.source = "typical dimensions of a small arm (shoulder height 0.30 m, upper arm 0.40 m, forearm 0.35 m, flange 0.10 m); not a product";
  t.convention = kStandard;
  const double a[6] = {0.0, 0.40, 0.0, 0.0, 0.0, 0.0};
  const double d[6] = {0.30, 0.0, 0.0, 0.35, 0.0, 0.10};
  const double alpha[6] = {kPi / 2.0, 0.0, kPi / 2.0, -kPi / 2.0, kPi / 2.0, 0.0};
  for (int i = 0; i < 6; ++i) {
    JointRow r;
    r.a = a[i];
    r.alpha = alpha[i];
    r.d = d[i];
    t.rows.push_back(r);
  }
  return t;
}

// Do the last three axes meet in a point: no offsets at joints 5 and 6, and no offset along joint 5?
inline bool has_spherical_wrist(const ArmTable& t) {
  if (t.rows.size() != 6) return false;
  return t.rows[4].a == 0.0 && t.rows[5].a == 0.0 && t.rows[4].d == 0.0 && t.rows[3].a == 0.0 && t.rows[2].a == 0.0;
}

inline double wrap_angle(double a) {
  while (a > kPi) a -= 2.0 * kPi;
  while (a <= -kPi) a += 2.0 * kPi;
  return a;
}

// Every joint solution that puts the tool at `target`, for the arm of elbow_arm's kind. Empty if the pose is out of reach.
inline std::vector<std::vector<double> > ik_spherical(const ArmTable& t, const Pose& target, const Pose& base, const Pose& tool) {
  std::vector<std::vector<double> > out;
  const double d1 = t.rows[0].d, a2 = t.rows[1].a, d4 = t.rows[3].d, d6 = t.rows[5].d;
  const Pose flange = compose(compose(inverse(base), target), inverse(tool));
  double pc[3];
  for (int k = 0; k < 3; ++k) pc[k] = flange.p[k] - d6 * flange.r[k][2];
  const double r = std::hypot(pc[0], pc[1]);
  const double yp = pc[2] - d1;
  (void)pc;  // TODO: the first joint is atan2 of the wrist centre (or that plus a half turn, with the radius negative); the elbow comes from sin(theta3) = (X^2 + Y^2 - a2^2 - d4^2) / (2 a2 d4), two angles; theta2 = atan2(Y, X) - atan2(-d4 cos(theta3), a2 + d4 sin(theta3)); the wrist is a Z Y Z Euler angle of R03 transposed times the flange's rotation, with its two solutions and its singular case
  (void)r;
  (void)yp;
  (void)d1;
  (void)a2;
  (void)d4;
  return out;
}

// Keeps the solutions that are inside the limits of the table.
inline std::vector<std::vector<double> > within_limits_only(const ArmTable& t, const std::vector<std::vector<double> >& sols) {
  std::vector<std::vector<double> > out;
  for (std::size_t i = 0; i < sols.size(); ++i)
    if (within_limits(t, sols[i])) out.push_back(sols[i]);
  return out;
}


#endif
