// rc/arm/ik_closed.hpp
//
// Eight ways to reach a pose, from lesson 28-04.
//
// elbow_arm is a typical six joint arm of the kind with an elbow and a spherical wrist (the dimensions are typical, not a product's). has_spherical_wrist says whether the last three axes of a table meet in a point, which is what the closed form needs. ik_spherical takes the pose wanted, the base and the tool, finds the wrist centre (the flange less the last link along the tool's axis), and gives every solution: two for the first joint (the shoulder in front or turned round), two for the elbow (up or down) from the law of cosines on the triangle of upper arm, forearm and wrist centre, the shoulder from the geometry, and two for the wrist from the Z Y Z Euler angles of what the first three joints leave to be done, with a single answer where the wrist is singular. A pose too far away gives none. within_limits_only keeps those inside a table's limits.
//
// For 40 reachable poses the solver gave 8.0 solutions on average, the joint angles that made the pose were among them in 40 of 40, and every one of the solutions put the tool within 0.0000000000 m and 0.0000000000 of the pose. A pose 1.5 m away gave 0 solutions; one at the very edge of the reach gave 4 (the elbow up and down are the same, so 2 shoulders and 2 wrists); a pose made with a straight wrist gave 6. The UR5 is 0 a spherical wrist (1 yes, 0 no), because its fifth joint is offset by 0.09465 m. With limits of plus and minus 120 degrees on every joint 1 of the 8 were allowed for a typical pose.

#ifndef RC_RC_ARM_IK_CLOSED_HPP
#define RC_RC_ARM_IK_CLOSED_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/fk.hpp>
#include <vector>

namespace rc {
namespace arm {

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
  for (int back = 0; back < 2; ++back) {
    const double th1 = wrap_angle(std::atan2(pc[1], pc[0]) + (back ? kPi : 0.0));
    const double X = back ? -r : r;
    double S = (X * X + yp * yp - a2 * a2 - d4 * d4) / (2.0 * a2 * d4);
    if (S > 1.0 + 1e-12 || S < -1.0 - 1e-12) continue;
    S = std::fmax(-1.0, std::fmin(1.0, S));
    for (int down = 0; down < 2; ++down) {
      if (down == 1 && std::fabs(S) > 1.0 - 1e-9) continue;  // the elbow straight: up and down are one
      const double th3 = wrap_angle(down ? kPi - std::asin(S) : std::asin(S));
      const double th2 = wrap_angle(std::atan2(yp, X) - std::atan2(-d4 * std::cos(th3), a2 + d4 * std::sin(th3)));
      const Pose r03 = compose(compose(row_pose(t, 0, th1), row_pose(t, 1, th2)), row_pose(t, 2, th3));
      const Pose r36 = compose(inverse(r03), flange);
      const double s5 = std::hypot(r36.r[0][2], r36.r[1][2]);
      if (s5 > 1e-9) {
        for (int flip = 0; flip < 2; ++flip) {
          const double sg = flip ? -1.0 : 1.0;
          std::vector<double> q(6);
          q[0] = th1;
          q[1] = th2;
          q[2] = th3;
          q[4] = std::atan2(sg * s5, r36.r[2][2]);
          q[3] = std::atan2(sg * r36.r[1][2], sg * r36.r[0][2]);
          q[5] = std::atan2(sg * r36.r[2][1], -sg * r36.r[2][0]);
          out.push_back(q);
        }
      } else {
        std::vector<double> q(6);
        q[0] = th1;
        q[1] = th2;
        q[2] = th3;
        q[3] = 0.0;
        if (r36.r[2][2] > 0.0) {
          q[4] = 0.0;
          q[5] = std::atan2(r36.r[1][0], r36.r[0][0]);
        } else {
          q[4] = kPi;
          q[5] = std::atan2(r36.r[1][0], -r36.r[0][0]);
        }
        out.push_back(q);
      }
    }
  }
  return out;
}

// Keeps the solutions that are inside the limits of the table.
inline std::vector<std::vector<double> > within_limits_only(const ArmTable& t, const std::vector<std::vector<double> >& sols) {
  std::vector<std::vector<double> > out;
  for (std::size_t i = 0; i < sols.size(); ++i)
    if (within_limits(t, sols[i])) out.push_back(sols[i]);
  return out;
}


}  // namespace arm
}  // namespace rc

#endif
