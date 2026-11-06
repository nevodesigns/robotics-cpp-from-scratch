#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// What a camera is, as numbers.
//
// A pinhole camera sends a point at (X, Y, Z) in front of it to a pixel by dividing by the
// depth and scaling by the focal length: u = fx * X / Z + cx, v = fy * Y / Z + cy. The focal
// length is in pixels, and the centre is where the optical axis meets the sensor.
//
// A real lens is not a pinhole. It bends light more towards the edge of the picture than
// the middle, so straight lines bow. The usual model scales a point's distance from the
// centre by one plus a series in the square of that distance. Barrel distortion, which wide
// lenses have, has a negative first coefficient and pulls the edges in.
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
// front of the camera: X / Z and Y / Z. A pixel is a direction, not a position, and the
// depth along it is not in the image.
struct Direction {
  double x = 0.0;
  double y = 0.0;
};

// Where a point in front of the camera lands in the picture. `visible` is false for a point
// at or behind the camera, which a division by the depth would send to a pixel anyway: a
// point behind the lens does not appear mirrored, it does not appear.
inline Pixel project(const Intrinsics& k, double X, double Y, double Z, bool& visible) {
  Pixel p;
  visible = Z > 0.0;
  if (!visible) return p;

  const double x = X / Z;
  const double y = Y / Z;
  const double r2 = x * x + y * y;
  const double scale = 1.0 + k.k1 * r2 + k.k2 * r2 * r2;
  p.u = k.fx * x * scale + k.cx;
  p.v = k.fy * y * scale + k.cy;
  return p;
}

// The direction a pixel looks along, with the lens's bending taken out.
//
// Projection is easy and its inverse is not: the scale depends on the radius of the point
// that is being looked for. It is found by guessing the point is where the pixel says, working
// out the scale there, and correcting, and repeating. Each pass is closer, and a handful is
// enough near the middle and more at the corner.
inline Direction unproject(const Intrinsics& k, const Pixel& p, int iterations = 20) {
  const double xd = (p.u - k.cx) / k.fx;
  const double yd = (p.v - k.cy) / k.fy;

  double x = xd;
  double y = yd;
  for (int i = 0; i < iterations; ++i) {
    const double r2 = x * x + y * y;
    const double scale = 1.0 + k.k1 * r2 + k.k2 * r2 * r2;
    x = xd / scale;
    y = yd / scale;
  }
  return Direction{x, y};
}

// How far away something of a known width is, from how wide it looks: similar triangles.
inline double distance_from_width(const Intrinsics& k, double real_width, double pixel_width) {
  if (pixel_width <= 0.0) return 0.0;
  return k.fx * real_width / pixel_width;
}

#endif  // LESSON_SOLUTION_HPP
