#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// What a camera is, as numbers: focal lengths in pixels, the centre where the optical axis
// meets the sensor, and two coefficients for the lens's bending of straight lines.
struct Intrinsics {
  double fx = 500.0;
  double fy = 500.0;
  double cx = 320.0;
  double cy = 240.0;
  double k1 = 0.0;
  double k2 = 0.0;
};

struct Pixel {
  double u = 0.0;
  double v = 0.0;
};

// The direction a pixel looks along, as the point where it crosses a plane one metre in
// front of the camera: X / Z and Y / Z.
struct Direction {
  double x = 0.0;
  double y = 0.0;
};

// Where a point in front of the camera lands in the picture.
inline Pixel project(const Intrinsics& k, double X, double Y, double Z, bool& visible) {
  // TODO: a point at or behind the camera does not appear: visible is false and the pixel
  // is the default. A division by a depth of zero or less would send it to a pixel anyway.
  //
  // Otherwise divide by the depth, then bend: x = X / Z, y = Y / Z, r2 = x*x + y*y, and the
  // lens scales both by 1 + k1 r2 + k2 r2^2. Then the pixel is
  //   u = fx * x * scale + cx
  //   v = fy * y * scale + cy
  (void)k;
  (void)X;
  (void)Y;
  (void)Z;
  visible = false;
  return Pixel{};
}

// The direction a pixel looks along, with the lens's bending taken out.
inline Direction unproject(const Intrinsics& k, const Pixel& p, int iterations = 20) {
  // TODO: the bend depends on the radius of the point being looked for, so it is found by
  // iterating. Start from the distorted coordinates xd = (u - cx) / fx and yd = (v - cy) / fy.
  // Guess x = xd and y = yd. Then, `iterations` times: work out r2 and the scale at the
  // guess, 1 + k1 r2 + k2 r2^2, and correct the guess to x = xd / scale, y = yd / scale.
  //
  // The scale at the guess is used, not at the answer, which is what is not known yet.
  (void)k;
  (void)p;
  (void)iterations;
  return Direction{};
}

// How far away something of a known width is, from how wide it looks: similar triangles.
inline double distance_from_width(const Intrinsics& k, double real_width, double pixel_width) {
  // TODO: the focal length times the real width over the width in pixels. A width of nothing
  // or less is no distance: zero.
  (void)k;
  (void)real_width;
  (void)pixel_width;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
