#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/frames.hpp>
#include <vector>

using namespace rc::arm;

struct Intrinsics {
  double fx = 600.0;
  double fy = 600.0;
  double cx = 320.0;
  double cy = 240.0;
};

// A pixel (u right, v down) and a depth along the optical axis to a point in the camera frame.
inline void back_project(const Intrinsics& k, double u, double v, double depth, double out[3]) {
  (void)k;  // TODO: x = (u - cx) z / fx, y = (v - cy) z / fy, z = the depth
  (void)u;
  (void)v;
  out[0] = 0.0;
  out[1] = 0.0;
  out[2] = depth;
}

struct Cloud3 {
  std::vector<double> p;  // x y z x y z ...
  std::size_t size() const { return p.size() / 3; }
  void add(double x, double y, double z) {
    p.push_back(x);
    p.push_back(y);
    p.push_back(z);
  }
};

struct FitResult {
  bool ok = false;
  Pose t;               // maps points in the first cloud onto the second
  double rms = 0.0;     // metres
};

inline double det3(const double m[3][3]) {
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

// The transform T with b close to T a, for corresponding clouds a (in the camera) and b (in the base).
inline FitResult fit_rigid(const Cloud3& a, const Cloud3& b) {
  FitResult out;
  const std::size_t n = a.size();
  if (n < 4 || b.size() != n) return out;
  double ca[3] = {0, 0, 0}, cb[3] = {0, 0, 0};
  for (std::size_t i = 0; i < n; ++i)
    for (int k = 0; k < 3; ++k) {
      ca[k] += a.p[3 * i + k];
      cb[k] += b.p[3 * i + k];
    }
  for (int k = 0; k < 3; ++k) {
    ca[k] /= static_cast<double>(n);
    cb[k] /= static_cast<double>(n);
  }
  double m[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  for (std::size_t i = 0; i < n; ++i)
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) m[r][c] += (b.p[3 * i + r] - cb[r]) * (a.p[3 * i + c] - ca[c]);
  // The polar factor of m, by Newton's iteration x <- (x + x^-T) / 2.
  double x[3][3];
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) x[r][c] = m[r][c];
  for (int it = 0; it < 60; ++it) {
    const double d = det3(x);
    if (std::fabs(d) < 1e-12) return out;  // flat or a line: the rotation is not determined
    double y[3][3];
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) {
        const int r1 = (r + 1) % 3, r2 = (r + 2) % 3, c1 = (c + 1) % 3, c2 = (c + 2) % 3;
        const double cof = x[r1][c1] * x[r2][c2] - x[r1][c2] * x[r2][c1];
        y[r][c] = 0.5 * (x[r][c] + cof / d);
      }
    double change = 0.0;
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) {
        change += std::fabs(y[r][c] - x[r][c]);
        x[r][c] = y[r][c];
      }
    if (change < 1e-13) break;
  }
  if (det3(x) < 0.0) return out;  // a mirror image, not a rotation
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) out.t.r[r][c] = x[r][c];
  for (int r = 0; r < 3; ++r) out.t.p[r] = cb[r] - (x[r][0] * ca[0] + x[r][1] * ca[1] + x[r][2] * ca[2]);
  double sum = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    double in[3] = {a.p[3 * i], a.p[3 * i + 1], a.p[3 * i + 2]}, o[3];
    apply(out.t, in, o);
    for (int k = 0; k < 3; ++k) sum += (o[k] - b.p[3 * i + k]) * (o[k] - b.p[3 * i + k]);
  }
  out.rms = std::sqrt(sum / static_cast<double>(n));
  out.ok = true;
  return out;
}

// A point seen by the camera, in the base frame.
inline void camera_to_base(const Pose& base_from_camera, const double in[3], double out[3]) { apply(base_from_camera, in, out); }


#endif
