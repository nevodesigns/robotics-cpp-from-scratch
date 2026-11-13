#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/arm/dynamics3d.hpp>
#include <vector>

using namespace rc::arm;

struct Capsule {
  double a[3];
  double b[3];
  double r;
};

struct Sphere {
  double c[3];
  double r;
};

struct Box {
  double lo[3];
  double hi[3];
};

struct Scene {
  std::vector<Sphere> spheres;
  std::vector<Box> boxes;
  bool has_floor = false;
  double floor_z = 0.0;
};

// The distance between two segments (Ericson's closest point of two segments).
inline double seg_seg_distance(const double p1[3], const double q1[3], const double p2[3], const double q2[3]) {
  double d1[3], d2[3], r[3];
  for (int k = 0; k < 3; ++k) {
    d1[k] = q1[k] - p1[k];
    d2[k] = q2[k] - p2[k];
    r[k] = p1[k] - p2[k];
  }
  const double a = d1[0] * d1[0] + d1[1] * d1[1] + d1[2] * d1[2];
  const double e = d2[0] * d2[0] + d2[1] * d2[1] + d2[2] * d2[2];
  const double f = d2[0] * r[0] + d2[1] * r[1] + d2[2] * r[2];
  double s = 0.0, t = 0.0;
  if (a <= 1e-12 && e <= 1e-12) {
    s = t = 0.0;
  } else if (a <= 1e-12) {
    t = std::fmax(0.0, std::fmin(1.0, f / e));
  } else {
    const double c = d1[0] * r[0] + d1[1] * r[1] + d1[2] * r[2];
    if (e <= 1e-12) {
      s = std::fmax(0.0, std::fmin(1.0, -c / a));
    } else {
      const double b = d1[0] * d2[0] + d1[1] * d2[1] + d1[2] * d2[2];
      const double denom = a * e - b * b;
      s = denom > 1e-12 ? std::fmax(0.0, std::fmin(1.0, (b * f - c * e) / denom)) : 0.0;
      t = (b * s + f) / e;
      if (t < 0.0) {
        t = 0.0;
        s = std::fmax(0.0, std::fmin(1.0, -c / a));
      } else if (t > 1.0) {
        t = 1.0;
        s = std::fmax(0.0, std::fmin(1.0, (b - c) / a));
      }
    }
  }
  double dist2 = 0.0;
  for (int k = 0; k < 3; ++k) {
    const double diff = (p1[k] + d1[k] * s) - (p2[k] + d2[k] * t);
    dist2 += diff * diff;
  }
  return std::sqrt(dist2);
}

inline double point_box_distance(const double p[3], const Box& b) {
  double d2 = 0.0;
  for (int k = 0; k < 3; ++k) {
    const double c = std::fmax(b.lo[k], std::fmin(b.hi[k], p[k]));
    d2 += (p[k] - c) * (p[k] - c);
  }
  return std::sqrt(d2);
}

// The distance from a segment to a box: the distance of the point at t along the segment is convex in t, so a ternary search finds its smallest.
inline double seg_box_distance(const double a[3], const double b[3], const Box& box) {
  double lo = 0.0, hi = 1.0;
  for (int it = 0; it < 60; ++it) {
    const double m1 = lo + (hi - lo) / 3.0, m2 = hi - (hi - lo) / 3.0;
    double p1[3], p2[3];
    for (int k = 0; k < 3; ++k) {
      p1[k] = a[k] + (b[k] - a[k]) * m1;
      p2[k] = a[k] + (b[k] - a[k]) * m2;
    }
    if (point_box_distance(p1, box) < point_box_distance(p2, box)) {
      hi = m2;
    } else {
      lo = m1;
    }
  }
  const double t = 0.5 * (lo + hi);
  double p[3];
  for (int k = 0; k < 3; ++k) p[k] = a[k] + (b[k] - a[k]) * t;
  return point_box_distance(p, box);
}

// The signed distance (negative if they overlap) of a capsule to a sphere, a box and a floor.
inline double capsule_sphere(const Capsule& c, const Sphere& s) { return seg_seg_distance(c.a, c.b, s.c, s.c) - c.r - s.r; }
inline double capsule_box(const Capsule& c, const Box& b) { return seg_box_distance(c.a, c.b, b) - c.r; }
inline double capsule_floor(const Capsule& c, double floor_z) { return std::fmin(c.a[2], c.b[2]) - c.r - floor_z; }
inline double capsule_capsule(const Capsule& x, const Capsule& y) { return seg_seg_distance(x.a, x.b, y.a, y.b) - x.r - y.r; }

// One capsule for every link that has a length, with the given radius for each link, the last carried on to the tool tip.
inline std::vector<Capsule> arm_capsules(const ArmTable& t, const std::vector<double>& q, const std::vector<double>& radii, double tool_length) {
  std::vector<Capsule> out;
  const std::vector<Pose> frames = link_frames(t, q, Pose());
  for (std::size_t i = 0; i < t.rows.size(); ++i) {
    Capsule c;
    for (int k = 0; k < 3; ++k) {
      c.a[k] = i == 0 ? 0.0 : frames[i - 1].p[k];
      c.b[k] = frames[i].p[k];
    }
    if (i + 1 == t.rows.size())
      for (int k = 0; k < 3; ++k) c.b[k] += frames[i].r[k][2] * tool_length;
    c.r = radii[i];
    const double len2 = (c.b[0] - c.a[0]) * (c.b[0] - c.a[0]) + (c.b[1] - c.a[1]) * (c.b[1] - c.a[1]) + (c.b[2] - c.a[2]) * (c.b[2] - c.a[2]);
    if (len2 > 1e-8) out.push_back(c);
  }
  return out;
}

struct Clearance {
  double d = 1e9;      // the smallest signed distance, metres
  int capsule = -1;    // which capsule of the arm
  int other = -1;      // the obstacle (a count over spheres, then boxes), -1 for the floor, -2 for another link
};

// The smallest signed distance between the arm and the scene, and, if asked, between links that do not share a joint (those more than one apart in the list of capsules).
inline Clearance scene_distance(const std::vector<Capsule>& caps, const Scene& scene, bool self) {
  Clearance out;
  for (std::size_t i = 0; i < caps.size(); ++i) {
    for (std::size_t s = 0; s < scene.spheres.size(); ++s) {
      const double d = capsule_sphere(caps[i], scene.spheres[s]);
      if (d < out.d) {
        out.d = d;
        out.capsule = static_cast<int>(i);
        out.other = static_cast<int>(s);
      }
    }
    for (std::size_t b = 0; b < scene.boxes.size(); ++b) {
      const double d = capsule_box(caps[i], scene.boxes[b]);
      if (d < out.d) {
        out.d = d;
        out.capsule = static_cast<int>(i);
        out.other = static_cast<int>(scene.spheres.size() + b);
      }
    }
    if (scene.has_floor && i > 0) {  // the first capsule is the base column, which stands on the floor
      const double d = capsule_floor(caps[i], scene.floor_z);
      if (d < out.d) {
        out.d = d;
        out.capsule = static_cast<int>(i);
        out.other = -1;
      }
    }
    if (self) {
      (void)caps;  // TODO: for every pair of capsules more than one apart in the list (the ones next to each other share a joint and always touch), the signed distance between them; keep the smallest
    }
  }
  return out;
}

inline bool collides(const std::vector<Capsule>& caps, const Scene& scene, bool self, double margin) { return scene_distance(caps, scene, self).d < margin; }

// How many pairs of capsules are asked about by the self check.
inline int self_pairs(std::size_t n_capsules) {
  int n = 0;
  for (std::size_t i = 0; i < n_capsules; ++i)
    for (std::size_t j = i + 2; j < n_capsules; ++j) ++n;
  return n;
}


#endif
