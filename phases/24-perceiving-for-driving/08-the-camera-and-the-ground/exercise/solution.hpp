#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <limits>

struct Camera {
  double fx = 500.0;  // focal length in pixels along the rows of the image (u)
  double fy = 500.0;  // and down the columns (v)
  double cx = 320.0;  // the principal point
  double cy = 240.0;
};

// The point on level ground that pixel (u, v) of a camera looks at, in the frame of the camera's foot (x ahead, y to the left), for a camera `height` above the ground and tipped down by `pitch` radians. False if the pixel looks at or above the horizon.
inline bool pixel_to_ground(const Camera& cam, double height, double pitch, double u, double v, double& x, double& y) {
  // The direction of the ray: forward 1, to the left (cx - u) / fx, up (cy - v) / fy, in the frame of a camera that is level; the camera is tipped down by pitch.
  const double left = (cam.cx - u) / cam.fx;
  const double up = (cam.cy - v) / cam.fy;
  (void)left;  // TODO: tipped down by the pitch the ray is (cos(pitch) + up sin(pitch), left, -sin(pitch) + up cos(pitch)) in the vehicle's frame; it meets the ground when its height has fallen by `height`, at t = -height / its z; the point is t times its x and y
  (void)up;
  (void)height;
  (void)pitch;
  x = y = 0.0;
  return false;
}

// The pixel that a point on the ground (x ahead, y left of the camera's foot) falls on. False for a point that is not in front of the camera.
inline bool ground_to_pixel(const Camera& cam, double height, double pitch, double x, double y, double& u, double& v) {
  const double dz = -height;
  const double forward = x * std::cos(pitch) - dz * std::sin(pitch);
  const double up = x * std::sin(pitch) + dz * std::cos(pitch);
  if (forward <= 1e-9) return false;
  u = cam.cx - cam.fx * y / forward;
  v = cam.cy - cam.fy * up / forward;
  return true;
}

// The row of the horizon: where the ground at an infinite distance falls.
inline double horizon_row(const Camera& cam, double pitch) { return cam.cy - cam.fy * std::tan(pitch); }

// How much ground one row spans at a row v, in the middle column: the difference in distance between the rows v and v + 1. Infinity at the horizon.
inline double ground_step(const Camera& cam, double height, double pitch, double v) {
  double x0 = 0.0, y0 = 0.0, x1 = 0.0, y1 = 0.0;
  if (!pixel_to_ground(cam, height, pitch, cam.cx, v, x0, y0)) return std::numeric_limits<double>::infinity();
  if (!pixel_to_ground(cam, height, pitch, cam.cx, v - 0.5, x1, y1)) return std::numeric_limits<double>::infinity();
  double x2 = 0.0, y2 = 0.0;
  if (!pixel_to_ground(cam, height, pitch, cam.cx, v + 0.5, x2, y2)) return std::numeric_limits<double>::infinity();
  return std::fabs(x1 - x2);
}


#endif
