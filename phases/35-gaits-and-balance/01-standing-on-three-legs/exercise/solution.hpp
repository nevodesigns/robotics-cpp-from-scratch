#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// The distance from the point p to the nearest edge of the convex polygon pts (n corners, in order, either way round): positive inside, negative outside.
inline double polygon_margin(const double pts[][2], int n, const double p[2]) {
  (void)pts;  // TODO: for each edge from corner i to the next, the signed distance of p from the edge's line is the cross product of the edge with (p - corner i) over the edge's length, positive on the left for a polygon taken anticlockwise (flip the sign if the corners go clockwise: the sign of the area); the margin is the least over the edges
  (void)n;
  (void)p;
  return 0.0;
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


#endif
