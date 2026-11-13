#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/fk.hpp>
#include <vector>

using namespace rc::arm;

struct Axis {
  double o[3];  // a point on the axis, in the base frame
  double z[3];  // the axis, a unit vector
};

// The axis of every joint in the base frame, by the rule of a convention.
inline std::vector<Axis> joint_axes(const ArmTable& t, const std::vector<double>& q, const Pose& base, DhConvention rule) {
  std::vector<Axis> out;
  Pose acc = base;
  for (std::size_t i = 0; i < t.rows.size(); ++i) {
    const Pose before = acc;
    acc = compose(acc, row_pose(t, i, q[i]));
    const Pose& at = rule == kStandard ? before : acc;
    Axis a;
    for (int k = 0; k < 3; ++k) {
      a.o[k] = at.p[k];
      a.z[k] = at.r[k][2];
    }
    out.push_back(a);
  }
  return out;
}

struct Jacobian {
  std::size_t n = 0;
  double m[6][7];
  Jacobian() {
    for (int i = 0; i < 6; ++i)
      for (int j = 0; j < 7; ++j) m[i][j] = 0.0;
  }
};

inline void cross3(const double a[3], const double b[3], double out[3]) {
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
}

// The geometric Jacobian at the tool (the flange plus a tool offset), in the base frame: rows 0 to 2 linear, 3 to 5 angular.
inline Jacobian jacobian(const ArmTable& t, const std::vector<double>& q, const Pose& base, const Pose& tool, DhConvention rule) {
  Jacobian j;
  j.n = t.rows.size();
  const Pose tip = forward(t, q, base, tool);
  const std::vector<Axis> ax = joint_axes(t, q, base, rule);
  for (std::size_t i = 0; i < j.n; ++i) {
    (void)ax;  // TODO: the linear part of column i is the axis z cross (the tool point minus the point on the axis); the angular part is the axis z
    (void)tip;
  }
  return j;
}

// The tool's velocity (three linear, three angular, in the base frame) for joint rates.
inline void tool_twist(const Jacobian& j, const std::vector<double>& qdot, double out[6]) {
  for (int r = 0; r < 6; ++r) {
    out[r] = 0.0;
    for (std::size_t c = 0; c < j.n; ++c) out[r] += j.m[r][c] * qdot[c];
  }
}

// The joint torques that hold a wrench (force, then moment, in the base frame) at the tool: the transpose of the Jacobian times it.
inline std::vector<double> joint_torques(const Jacobian& j, const double wrench[6]) {
  std::vector<double> tau(j.n, 0.0);
  for (std::size_t c = 0; c < j.n; ++c)
    for (int r = 0; r < 6; ++r) tau[c] += j.m[r][c] * wrench[r];
  return tau;
}

// The same matrix by differences of the forward kinematics: the position by the change of the tool's place, the rotation by the small turn between the two poses.
inline Jacobian numeric_jacobian(const ArmTable& t, const std::vector<double>& q, const Pose& base, const Pose& tool) {
  Jacobian j;
  j.n = t.rows.size();
  const double h = 1e-6;
  for (std::size_t i = 0; i < j.n; ++i) {
    std::vector<double> qa = q, qb = q;
    qa[i] += h;
    qb[i] -= h;
    const Pose a = forward(t, qa, base, tool);
    const Pose b = forward(t, qb, base, tool);
    for (int k = 0; k < 3; ++k) j.m[k][i] = (a.p[k] - b.p[k]) / (2.0 * h);
    // a rotation times the transpose of the other is a small turn about the axis we want: the antisymmetric part is the axis times its angle.
    double d[3][3];
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) {
        d[r][c] = 0.0;
        for (int k = 0; k < 3; ++k) d[r][c] += a.r[r][k] * b.r[c][k];
      }
    j.m[3][i] = 0.5 * (d[2][1] - d[1][2]) / (2.0 * h);
    j.m[4][i] = 0.5 * (d[0][2] - d[2][0]) / (2.0 * h);
    j.m[5][i] = 0.5 * (d[1][0] - d[0][1]) / (2.0 * h);
  }
  return j;
}

// The largest difference between two Jacobians.
inline double jacobian_gap(const Jacobian& a, const Jacobian& b) {
  double w = 0.0;
  for (int r = 0; r < 6; ++r)
    for (std::size_t c = 0; c < a.n; ++c) w = std::fmax(w, std::fabs(a.m[r][c] - b.m[r][c]));
  return w;
}


#endif
