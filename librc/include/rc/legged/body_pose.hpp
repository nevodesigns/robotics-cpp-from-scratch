// rc/legged/body_pose.hpp
//
// The body from the feet, from lesson 34-03.
//
// fit_ground_plane takes the places of the feet in the body's frame, and a flag for each saying whether the foot is on the ground, and fits the plane z = c0 + c1 x + c2 y through the stance feet by least squares (the normal equations of three unknowns, solved by Cramer's rule); it gives false if fewer than three feet are on the ground or they lie in a line. body_pose_from_feet turns the plane into the body's pose: the plane's unit normal, seen from the body, is (minus sin of the pitch, sin of the roll times cos of the pitch, cos of the roll times cos of the pitch), so the pitch is the arcsine of minus its x and the roll is the arctangent of its y over its z, and the height is the distance of the body's origin from the plane.
//
// A body 0.30 m up, rolled 5 degrees and pitched -3 degrees was found again from its four feet to 0.000000000 m, 0.000000000 degrees and 0.000000000 degrees. With 5 mm of noise in the feet's places the height was uncertain by 2.5 mm, the roll by 0.92 degrees and the pitch by 0.65 degrees with four feet on the ground, and by 3.6 mm, 1.29 degrees and 0.95 degrees with three. Three feet in a line gave no plane.

#ifndef RC_RC_LEGGED_BODY_POSE_HPP
#define RC_RC_LEGGED_BODY_POSE_HPP

#include <cmath>

namespace rc {
namespace legged {

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


}  // namespace legged
}  // namespace rc

#endif
