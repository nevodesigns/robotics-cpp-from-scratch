#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/mobile/base_reach.hpp>
#include <rc/mobile/mm_frames.hpp>

using namespace rc::arm;
using namespace rc::mobile;

struct Obstacle {
  double x = 0.0;
  double y = 0.0;
  double r = 0.0;
};

struct ParkParams {
  double base_radius = 0.35;   // m
  double clearance = 0.10;     // m, the least gap to an obstacle
  double min_margin = 0.05;    // m, the least distance inside the ring's edges
  double w_margin = 1.0;
  double w_travel = 0.3;       // per metre
  double w_clear = 0.5;        // per metre of clearance, up to the cap
  double clear_cap = 0.5;
  int n_bearings = 36;
  int n_radii = 7;
};

struct ParkChoice {
  bool found = false;
  BasePose base;
  double score = -1e18;
  double margin = 0.0;        // the margin in the ring over the tolerance, 1 in the middle
  int evaluated = 0;          // places tried
  int free_places = 0;        // places that were clear and had the margin
};

// The base place for the arm's axis at (ax, ay) with the base turned to `heading`.
inline BasePose base_for_axis(const MobileManipulator& m, double ax, double ay, double heading) {
  BasePose b;
  b.heading = heading;
  const double c = std::cos(heading), s = std::sin(heading);
  b.x = ax - (c * m.mount.p[0] - s * m.mount.p[1]);
  b.y = ay - (s * m.mount.p[0] + c * m.mount.p[1]);
  return b;
}

// The best place to park for a target (x, y), from `current`, among obstacles; found is false if none is clear and in reach.
inline ParkChoice choose_park(const MobileManipulator& m, const Annulus& ring, const double target[2], const BasePose& current, const Obstacle* obs, int n_obs, const ParkParams& p) {
  ParkChoice out;
  const double tol = park_tolerance(ring);
  const double kPi2 = 6.283185307179586;
  for (int i = 0; i < p.n_bearings; ++i) {
    const double phi = kPi2 * i / p.n_bearings;
    for (int j = 0; j < p.n_radii; ++j) {
      const double lo = ring.r_in + p.min_margin, hi = ring.r_out - p.min_margin;
      const double d = p.n_radii > 1 ? lo + (hi - lo) * j / (p.n_radii - 1) : 0.5 * (lo + hi);
      ++out.evaluated;
      (void)phi;  // TODO: the arm's axis is the target plus d at the bearing phi; the base faces the target (heading phi + pi), its centre from base_for_axis; drop it if its clearance to any obstacle (centre distance less the obstacle's radius and the base's) is under p.clearance; else score = w_margin * margin - w_travel * travel + w_clear * min(clearance, cap), the margin being the smaller of d - r_in and r_out - d over the tolerance; keep the best (and count the free places)
      (void)d;
      (void)obs;
      (void)n_obs;
      (void)current;
      (void)tol;
    }
  }
  return out;
}


#endif
