#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct Vec3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

// Row major: m[row][column]. Each column is where one axis ends up, which is
// the useful way to read any rotation matrix.
struct Mat3 {
  double m[3][3] = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
};

inline double length(const Vec3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

inline double dot(const Vec3& a, const Vec3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Each of these turns about one axis. Remember that a column of the matrix is
// where one axis ends up, which is a useful check on the signs.
inline Mat3 rotation_x(double angle) {
  // TODO
  (void)angle;
  return Mat3{};
}

inline Mat3 rotation_y(double angle) {
  // TODO: mind the signs on this one. The minus sits in a different place from
  // the other two, which is not a mistake in the textbook.
  (void)angle;
  return Mat3{};
}

inline Mat3 rotation_z(double angle) {
  // TODO
  (void)angle;
  return Mat3{};
}

inline Mat3 multiply(const Mat3& a, const Mat3& b) {
  // TODO: row of a against column of b.
  (void)a; (void)b;
  return Mat3{};
}

inline Vec3 apply(const Mat3& m, const Vec3& v) {
  // TODO
  (void)m; (void)v;
  return Vec3{};
}

// For a rotation the transpose is the inverse, which is what being orthonormal
// buys. No inversion algorithm and no division.
inline Mat3 transposed(const Mat3& m) {
  // TODO: for a rotation this is also the inverse, which is what orthonormality
  // buys you. No inversion algorithm needed.
  (void)m;
  return Mat3{};
}

inline Vec3 column(const Mat3& m, int index) {
  return Vec3{m.m[0][index], m.m[1][index], m.m[2][index]};
}

// The property that makes a matrix a rotation rather than any other matrix:
// every column is a unit vector and every pair of columns is perpendicular.
// That is exactly the statement that lengths and angles survive the transform.
inline bool is_orthonormal(const Mat3& m, double tolerance) {
  // TODO: every column has length one, and every pair of columns is
  // perpendicular. That is the definition of a rotation, and it is the property
  // that drift destroys.
  (void)m; (void)tolerance;
  return false;
}

// The aerospace convention, and the one ROS uses. Rotation does not commute, so
// the order is part of the definition rather than a detail: a different order is
// a different orientation.
inline Mat3 from_rpy(double roll, double pitch, double yaw) {
  // TODO: Rz(yaw) * Ry(pitch) * Rx(roll), in that order.
  //
  // The order is part of the definition rather than a detail. Rotation does not
  // commute, so a different order describes a different orientation.
  (void)roll; (void)pitch; (void)yaw;
  return Mat3{};
}

#endif  // LESSON_SOLUTION_HPP
