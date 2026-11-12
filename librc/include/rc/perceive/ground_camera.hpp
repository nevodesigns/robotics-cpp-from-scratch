// rc/perceive/ground_camera.hpp
//
// The camera and the ground, from lesson 24-08.
//
// A Camera is a focal length in pixels in each direction and a principal point. pixel_to_ground gives, for a camera at a height h above level ground, tipped down by a pitch, the point on the ground (x ahead, y to the left of the camera) that a pixel sees, or false for a pixel at or above the horizon, which sees no ground. ground_to_pixel is the inverse. horizon_row is the row of the horizon, cy - fy tan(pitch). ground_step is the length of ground that one pixel row spans at a row, which is how well the camera can tell distance there.
//
// For a camera 0.30 m up, tipped down 15 degrees, with a focal length of 500 pixels in an image of 640 by 480, the horizon was at row 106, the centre row looked at the ground 1.12 m ahead, row 140 at 4.65 m and row 120 at 11.42 m; one row at those ranges spanned 0.0090 m, 0.139 m and 0.82 m; and a pitch that was 1 degree wrong gave 37 percent error in the range of row 140, a height 10 percent wrong 10 percent.

#ifndef RC_RC_PERCEIVE_GROUND_CAMERA_HPP
#define RC_RC_PERCEIVE_GROUND_CAMERA_HPP

#include <algorithm>
#include <cmath>
#include <limits>

namespace rc {
namespace perceive {

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
  const double dx = std::cos(pitch) + up * std::sin(pitch);
  const double dz = -std::sin(pitch) + up * std::cos(pitch);
  if (dz >= -1e-12) return false;
  const double t = -height / dz;
  x = t * dx;
  y = t * left;
  return true;
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


}  // namespace perceive
}  // namespace rc

#endif
