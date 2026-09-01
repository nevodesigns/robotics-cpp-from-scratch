// rc/math/rotation.hpp
//
// The rotation matrix from lesson 06-02, graduated.
//
// Three by three, orthonormal, and the measured reason the next lesson exists:
// composing rotations repeatedly walks the matrix away from being a rotation at
// all, and reading three angles back out of one has orientations it cannot tell
// apart. Both were measured rather than asserted in that lesson.

#ifndef RC_MATH_ROTATION_HPP
#define RC_MATH_ROTATION_HPP

#include <cmath>

#include <rc/math/vector.hpp>

namespace rc {
namespace math {

struct Mat3 {
  double m[3][3] = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
};



inline Mat3 rotation_x(double angle) {
  const double c = std::cos(angle);
  const double s = std::sin(angle);
  Mat3 r;
  r.m[0][0] = 1; r.m[0][1] = 0; r.m[0][2] = 0;
  r.m[1][0] = 0; r.m[1][1] = c; r.m[1][2] = -s;
  r.m[2][0] = 0; r.m[2][1] = s; r.m[2][2] = c;
  return r;
}

inline Mat3 rotation_y(double angle) {
  const double c = std::cos(angle);
  const double s = std::sin(angle);
  Mat3 r;
  r.m[0][0] = c;  r.m[0][1] = 0; r.m[0][2] = s;
  r.m[1][0] = 0;  r.m[1][1] = 1; r.m[1][2] = 0;
  r.m[2][0] = -s; r.m[2][1] = 0; r.m[2][2] = c;
  return r;
}

inline Mat3 rotation_z(double angle) {
  const double c = std::cos(angle);
  const double s = std::sin(angle);
  Mat3 r;
  r.m[0][0] = c; r.m[0][1] = -s; r.m[0][2] = 0;
  r.m[1][0] = s; r.m[1][1] = c;  r.m[1][2] = 0;
  r.m[2][0] = 0; r.m[2][1] = 0;  r.m[2][2] = 1;
  return r;
}

inline Mat3 multiply(const Mat3& a, const Mat3& b) {
  Mat3 out;
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      double sum = 0.0;
      for (int k = 0; k < 3; ++k) sum += a.m[row][k] * b.m[k][col];
      out.m[row][col] = sum;
    }
  }
  return out;
}

inline Vec3 apply(const Mat3& m, const Vec3& v) {
  return Vec3{m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z,
              m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z,
              m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z};
}

// For a rotation the transpose is the inverse, which is what being orthonormal
// buys. No inversion algorithm and no division.
inline Mat3 transposed(const Mat3& m) {
  Mat3 out;
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) out.m[row][col] = m.m[col][row];
  }
  return out;
}

inline Vec3 column(const Mat3& m, int index) {
  return Vec3{m.m[0][index], m.m[1][index], m.m[2][index]};
}

// The property that makes a matrix a rotation rather than any other matrix:
// every column is a unit vector and every pair of columns is perpendicular.
// That is exactly the statement that lengths and angles survive the transform.
inline bool is_orthonormal(const Mat3& m, double tolerance) {
  for (int i = 0; i < 3; ++i) {
    if (std::fabs(length(column(m, i)) - 1.0) > tolerance) return false;
    for (int j = i + 1; j < 3; ++j) {
      if (std::fabs(dot(column(m, i), column(m, j))) > tolerance) return false;
    }
  }
  return true;
}

// The aerospace convention, and the one ROS uses. Rotation does not commute, so
// the order is part of the definition rather than a detail: a different order is
// a different orientation.
inline Mat3 from_rpy(double roll, double pitch, double yaw) {
  return multiply(rotation_z(yaw), multiply(rotation_y(pitch), rotation_x(roll)));
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_ROTATION_HPP
