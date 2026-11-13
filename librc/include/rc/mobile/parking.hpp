// rc/mobile/parking.hpp
//
// Choosing where to park, from lesson 33-03.
//
// Obstacle is a circle on the ground. ParkParams holds the base's radius, the least clearance it must keep from an obstacle, the least margin it must have inside the ring of 33-02, the weights of the score (margin, travel, clearance) and how many bearings and distances are tried. choose_park tries the arm's axis at each of the distances (between the ring's radii, kept the margin away from both) and each of the bearings round the target, with the base facing the target; the base's centre follows from the mount. A place is dropped if it comes within the base's radius and the clearance of an obstacle. The rest are scored: the margin in the ring over the tolerance (so 1 is the middle of the ring), less the travel from the present place scaled, plus the clearance (up to a cap), each with its weight. It returns the best place, whether there was one, and how many it looked at.
//
// The target was at (2.0, 0.5) metres and the base started at the origin. With nothing in the way and the margin weighted most, the base was put at a bearing of -170 degrees from the target, 0.36 m from it in the ring, with a margin of 1.00 of the tolerance (1 is the middle of the ring). With margin and travel weighted differently the base travelled 1.24 m when travel was weighted most and 2.56 m when it was not weighted at all. With a row of obstacles across the near side the choice went round to a bearing of -30 degrees, and with obstacles all round the target 0 of the 252 places tried were free, so it found none.

#ifndef RC_RC_MOBILE_PARKING_HPP
#define RC_RC_MOBILE_PARKING_HPP

#include <cmath>
#include <rc/mobile/base_reach.hpp>
#include <rc/mobile/mm_frames.hpp>

namespace rc {
namespace mobile {

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
      const double ax = target[0] + d * std::cos(phi), ay = target[1] + d * std::sin(phi);
      const BasePose b = base_for_axis(m, ax, ay, phi + 3.141592653589793);
      double clear = 1e9;
      for (int k = 0; k < n_obs; ++k) {
        const double g = std::hypot(b.x - obs[k].x, b.y - obs[k].y) - obs[k].r - p.base_radius;
        if (g < clear) clear = g;
      }
      if (clear < p.clearance) continue;
      ++out.free_places;
      const double margin = (d - ring.r_in < ring.r_out - d ? d - ring.r_in : ring.r_out - d) / (tol > 1e-9 ? tol : 1e-9);
      const double travel = std::hypot(b.x - current.x, b.y - current.y);
      const double cl = clear < p.clear_cap ? clear : p.clear_cap;
      const double score = p.w_margin * margin - p.w_travel * travel + p.w_clear * cl;
      if (score > out.score) {
        out.score = score;
        out.base = b;
        out.margin = margin;
        out.found = true;
      }
    }
  }
  return out;
}


}  // namespace mobile
}  // namespace rc

#endif
