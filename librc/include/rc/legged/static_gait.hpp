// rc/legged/static_gait.hpp
//
// Standing on three legs, from lesson 35-01.
//
// polygon_margin takes the corners of a convex polygon in order (the feet on the ground, seen from above, in the body's frame) and a point (the centre of mass projected onto the ground) and gives the distance from the point to the nearest edge: positive if it is inside, negative if outside, zero on an edge. The corners may be given either way round. centroid is the mean of the corners, a point well inside the polygon. crawl_margin gives the margin for the body centred over the origin with leg `lifted` off the ground (the three feet that are left make a triangle), and crawl_margin_shifted the margin after the body has moved so that its centre of mass is over the middle of the triangle.
//
// A robot with its hips 0.19 m fore and aft and 0.12 m to each side, and its feet 0.15 m out, had a margin of 0.000 m with each of the four feet lifted in turn and the body centred (the centre falls on the diagonal between two feet), 0.031 m with the body shifted forward 5 cm with a hind foot lifted, and 0.078 m after moving the centre of mass to the middle of the three feet, a shift of 8.1 cm.

#ifndef RC_RC_LEGGED_STATIC_GAIT_HPP
#define RC_RC_LEGGED_STATIC_GAIT_HPP

#include <cmath>

namespace rc {
namespace legged {

// The distance from the point p to the nearest edge of the convex polygon pts (n corners, in order, either way round): positive inside, negative outside.
inline double polygon_margin(const double pts[][2], int n, const double p[2]) {
  double area2 = 0.0;
  for (int i = 0; i < n; ++i) {
    const int j = (i + 1) % n;
    area2 += pts[i][0] * pts[j][1] - pts[j][0] * pts[i][1];
  }
  const double orient = area2 >= 0.0 ? 1.0 : -1.0;
  double best = 1e18;
  for (int i = 0; i < n; ++i) {
    const int j = (i + 1) % n;
    const double ex = pts[j][0] - pts[i][0], ey = pts[j][1] - pts[i][1];
    const double len = std::sqrt(ex * ex + ey * ey);
    const double cross = ex * (p[1] - pts[i][1]) - ey * (p[0] - pts[i][0]);
    const double d = orient * cross / len;
    if (d < best) best = d;
  }
  return best;
}

// The mean of the corners.
inline void centroid(const double pts[][2], int n, double out[2]) {
  out[0] = out[1] = 0.0;
  for (int i = 0; i < n; ++i) {
    out[0] += pts[i][0];
    out[1] += pts[i][1];
  }
  out[0] /= n;
  out[1] /= n;
}

// The four feet's places seen from above (front left, front right, rear left, rear right), for hips hx fore and aft and feet fy to each side.
inline void default_feet(double hx, double fy, double feet[4][2]) {
  feet[0][0] = hx;
  feet[0][1] = fy;
  feet[1][0] = hx;
  feet[1][1] = -fy;
  feet[2][0] = -hx;
  feet[2][1] = fy;
  feet[3][0] = -hx;
  feet[3][1] = -fy;
}

// The three feet that are left when `lifted` is off the ground, in order round the polygon.
inline void support_triangle(const double feet[4][2], int lifted, double tri[3][2]) {
  const int order[4] = {0, 1, 3, 2};   // round the rectangle: front left, front right, rear right, rear left
  int k = 0;
  for (int i = 0; i < 4; ++i)
    if (order[i] != lifted) {
      tri[k][0] = feet[order[i]][0];
      tri[k][1] = feet[order[i]][1];
      ++k;
    }
}

// The margin with the centre of mass at com (over the ground) and `lifted` off the ground.
inline double crawl_margin(const double feet[4][2], int lifted, const double com[2]) {
  double tri[3][2];
  support_triangle(feet, lifted, tri);
  return polygon_margin(tri, 3, com);
}

// The margin after the centre of mass has been moved to the middle of the three feet that are left.
inline double crawl_margin_shifted(const double feet[4][2], int lifted, double* shift) {
  double tri[3][2], c[2];
  support_triangle(feet, lifted, tri);
  centroid(tri, 3, c);
  if (shift) *shift = std::sqrt(c[0] * c[0] + c[1] * c[1]);
  return polygon_margin(tri, 3, c);
}


}  // namespace legged
}  // namespace rc

#endif
