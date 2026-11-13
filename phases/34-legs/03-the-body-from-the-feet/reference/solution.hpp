#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct BodyPose {
  bool ok = false;
  double height = 0.0;   // m, of the body's origin above the ground
  double roll = 0.0;     // rad
  double pitch = 0.0;    // rad
};

// The plane z = c0 + c1 x + c2 y through the feet on the ground (in the body's frame), by least squares; false if fewer than three are on the ground or they are in a line.
inline bool fit_ground_plane(const double feet[4][3], const bool on_ground[4], double c[3]) {
  double a[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}, b[3] = {0, 0, 0};
  int n = 0;
  for (int i = 0; i < 4; ++i) {
    if (!on_ground[i]) continue;
    ++n;
    const double row[3] = {1.0, feet[i][0], feet[i][1]};
    for (int r = 0; r < 3; ++r) {
      for (int k = 0; k < 3; ++k) a[r][k] += row[r] * row[k];
      b[r] += row[r] * feet[i][2];
    }
  }
  if (n < 3) return false;
  const double d = a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
  if (std::fabs(d) < 1e-12) return false;
  for (int k = 0; k < 3; ++k) {
    double t[3][3];
    for (int r = 0; r < 3; ++r)
      for (int q = 0; q < 3; ++q) t[r][q] = q == k ? b[r] : a[r][q];
    const double dk = t[0][0] * (t[1][1] * t[2][2] - t[1][2] * t[2][1]) - t[0][1] * (t[1][0] * t[2][2] - t[1][2] * t[2][0]) + t[0][2] * (t[1][0] * t[2][1] - t[1][1] * t[2][0]);
    c[k] = dk / d;
  }
  return true;
}

// The body's height, roll and pitch above level ground from the places of its feet in its own frame.
inline BodyPose body_pose_from_feet(const double feet[4][3], const bool on_ground[4]) {
  BodyPose p;
  double c[3];
  if (!fit_ground_plane(feet, on_ground, c)) return p;
  const double n = std::sqrt(1.0 + c[1] * c[1] + c[2] * c[2]);
  const double nx = -c[1] / n, ny = -c[2] / n, nz = 1.0 / n;
  p.pitch = std::asin(-nx);
  p.roll = std::atan2(ny, nz);
  p.height = -c[0] / n;
  p.ok = true;
  return p;
}


#endif
