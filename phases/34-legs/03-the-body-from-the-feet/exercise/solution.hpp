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
  (void)feet;  // TODO: for the feet on the ground form the normal equations A c = b with rows (1, x, y) and the right side z (A is the sum of row times row transposed, b the sum of row times z), and solve the 3 by 3 system by Cramer's rule; false if fewer than three feet or the determinant is nearly zero
  (void)on_ground;
  c[0] = 0.0;
  c[1] = 0.0;
  c[2] = 0.0;
  return false;
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
