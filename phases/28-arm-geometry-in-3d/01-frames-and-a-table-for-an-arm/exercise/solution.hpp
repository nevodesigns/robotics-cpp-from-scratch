#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

struct Pose {
  double r[3][3];
  double p[3];
  Pose() {
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) r[i][j] = i == j ? 1.0 : 0.0;
      p[i] = 0.0;
    }
  }
};

inline Pose compose(const Pose& a, const Pose& b) {
  Pose c;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      c.r[i][j] = 0.0;
      for (int k = 0; k < 3; ++k) c.r[i][j] += a.r[i][k] * b.r[k][j];
    }
    c.p[i] = a.p[i];
    for (int k = 0; k < 3; ++k) c.p[i] += a.r[i][k] * b.p[k];
  }
  return c;
}

inline Pose inverse(const Pose& a) {
  Pose c;
  (void)a;  // TODO: the rotation is the transpose of a's; the translation is minus that rotation times a's translation
  return c;
}

inline void apply(const Pose& a, const double in[3], double out[3]) {
  for (int i = 0; i < 3; ++i) {
    out[i] = a.p[i];
    for (int k = 0; k < 3; ++k) out[i] += a.r[i][k] * in[k];
  }
}

inline Pose rot_x(double t) {
  Pose c;
  c.r[1][1] = std::cos(t);
  c.r[1][2] = -std::sin(t);
  c.r[2][1] = std::sin(t);
  c.r[2][2] = std::cos(t);
  return c;
}

inline Pose rot_y(double t) {
  Pose c;
  c.r[0][0] = std::cos(t);
  c.r[0][2] = std::sin(t);
  c.r[2][0] = -std::sin(t);
  c.r[2][2] = std::cos(t);
  return c;
}

inline Pose rot_z(double t) {
  Pose c;
  c.r[0][0] = std::cos(t);
  c.r[0][1] = -std::sin(t);
  c.r[1][0] = std::sin(t);
  c.r[1][1] = std::cos(t);
  return c;
}

inline Pose trans(double x, double y, double z) {
  Pose c;
  c.p[0] = x;
  c.p[1] = y;
  c.p[2] = z;
  return c;
}

// How far the rotation of a pose is from a rotation: the largest entry of R R^T minus the identity.
inline double rotation_error(const Pose& a) {
  double worst = 0.0;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      double s = 0.0;
      for (int k = 0; k < 3; ++k) s += a.r[i][k] * a.r[j][k];
      worst = std::fmax(worst, std::fabs(s - (i == j ? 1.0 : 0.0)));
    }
  return worst;
}

// Puts the columns of the rotation back to unit length and right angles (Gram-Schmidt on the columns).
inline Pose orthonormalised(const Pose& a) {
  Pose c = a;
  double col[3][3];
  for (int j = 0; j < 3; ++j)
    for (int i = 0; i < 3; ++i) col[j][i] = a.r[i][j];
  for (int j = 0; j < 3; ++j) {
    for (int k = 0; k < j; ++k) {
      double d = 0.0;
      for (int i = 0; i < 3; ++i) d += col[j][i] * col[k][i];
      for (int i = 0; i < 3; ++i) col[j][i] -= d * col[k][i];
    }
    double n = 0.0;
    for (int i = 0; i < 3; ++i) n += col[j][i] * col[j][i];
    n = std::sqrt(n);
    for (int i = 0; i < 3; ++i) col[j][i] /= n;
  }
  for (int j = 0; j < 3; ++j)
    for (int i = 0; i < 3; ++i) c.r[i][j] = col[j][i];
  return c;
}

// One row of a table in the standard convention: Rz(theta) Tz(d) Tx(a) Rx(alpha).
inline Pose dh_standard(double a, double alpha, double d, double theta) {
  return compose(compose(rot_z(theta), trans(0.0, 0.0, d)), compose(trans(a, 0.0, 0.0), rot_x(alpha)));
}

// One row in the modified convention: Rx(alpha) Tx(a) Rz(theta) Tz(d).
inline Pose dh_modified(double a, double alpha, double d, double theta) {
  return compose(compose(rot_x(alpha), trans(a, 0.0, 0.0)), compose(rot_z(theta), trans(0.0, 0.0, d)));
}

enum DhConvention { kStandard = 0, kModified = 1 };

struct JointRow {
  double a = 0.0;
  double alpha = 0.0;
  double d = 0.0;
  double theta_offset = 0.0;  // added to the joint angle
  double lo = -3.14159265358979323846;
  double hi = 3.14159265358979323846;
};

struct ArmTable {
  std::string name;
  std::string source;
  DhConvention convention = kStandard;
  std::vector<JointRow> rows;
};

inline const double kPi = 3.14159265358979323846;

// The UR5, standard convention. Numbers (metres, radians) as the maker publishes them.
inline ArmTable ur5() {
  ArmTable t;
  t.name = "UR5";
  t.source = "Universal Robots, DH parameters of the UR5 as published by the maker; check against the data sheet of the arm";
  t.convention = kStandard;
  const double a[6] = {0.0, -0.425, -0.39225, 0.0, 0.0, 0.0};
  const double d[6] = {0.089159, 0.0, 0.0, 0.10915, 0.09465, 0.0823};
  const double alpha[6] = {kPi / 2.0, 0.0, 0.0, kPi / 2.0, -kPi / 2.0, 0.0};
  for (int i = 0; i < 6; ++i) {
    JointRow r;
    r.a = a[i];
    r.alpha = alpha[i];
    r.d = d[i];
    r.lo = -2.0 * kPi;
    r.hi = 2.0 * kPi;
    t.rows.push_back(r);
  }
  return t;
}

// The Franka Emika Panda, modified convention. Numbers (metres, radians) and joint limits as its documentation gives them.
inline ArmTable panda() {
  ArmTable t;
  t.name = "Panda";
  t.source = "Franka Emika, kinematics and joint limits of the Panda as published in its documentation; check against the documentation of the arm";
  t.convention = kModified;
  const double a[7] = {0.0, 0.0, 0.0, 0.0825, -0.0825, 0.0, 0.088};
  const double d[7] = {0.333, 0.0, 0.316, 0.0, 0.384, 0.0, 0.0};
  const double alpha[7] = {0.0, -kPi / 2.0, kPi / 2.0, kPi / 2.0, -kPi / 2.0, kPi / 2.0, kPi / 2.0};
  const double lo[7] = {-2.8973, -1.7628, -2.8973, -3.0718, -2.8973, -0.0175, -2.8973};
  const double hi[7] = {2.8973, 1.7628, 2.8973, -0.0698, 2.8973, 3.7525, 2.8973};
  for (int i = 0; i < 7; ++i) {
    JointRow r;
    r.a = a[i];
    r.alpha = alpha[i];
    r.d = d[i];
    r.lo = lo[i];
    r.hi = hi[i];
    t.rows.push_back(r);
  }
  return t;
}


#endif
