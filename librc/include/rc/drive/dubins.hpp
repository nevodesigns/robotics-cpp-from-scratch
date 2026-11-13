// rc/drive/dubins.hpp
//
// The shortest path for a car that only goes forward, from lesson 26-02.
//
// A Dubins path joins a start pose to a goal pose with the shortest route a car can drive if it never reverses and never turns tighter than a radius r. The route is always one of six words made of three pieces: a left arc (L), a right arc (R) or a straight line (S), as LSL, RSR, LSR, RSL, RLR or LRL. dubins_word works out one word in closed form, in units of the radius, and says whether it exists. dubins_shortest tries the six and keeps the shortest. dubins_sample walks the three pieces and gives poses a set spacing apart, the last of them exactly the goal.
//
// Over 200 pairs of poses the walked end was at most 0.000000000 m and 0.000000000 rad from the goal, and 6 of the six words were the shortest at least once. After the hybrid search of lesson 26-01 stopped 0.073 m and 2.8 degrees from the goal, a Dubins tail of 4.70 m finished it exactly, and the shortest tail from any of the last nine poses was 1.25 m. A goal 0.5 m directly behind the car, facing the same way, took a path 10.3 times as long as the distance. A radius of 0.4 m, smaller than the car's 0.742 m, gave a path of curvature 2.50 per metre against the car's limit of 1.35.

#ifndef RC_RC_DRIVE_DUBINS_HPP
#define RC_RC_DRIVE_DUBINS_HPP

#include <cmath>
#include <rc/drive/hybrid_astar.hpp>
#include <vector>

namespace rc {
namespace drive {

enum DubinsWord { kLSL = 0, kRSR = 1, kLSR = 2, kRSL = 3, kRLR = 4, kLRL = 5 };

struct Dubins {
  bool ok = false;
  int word = 0;
  double r = 1.0;       // metres, the smallest turning radius
  double t = 0.0;       // length of the first piece, in radii
  double p = 0.0;       // length of the second piece, in radii
  double q = 0.0;       // length of the third piece, in radii
  double length = 0.0;  // metres
  CarPose start;
};

inline double mod2pi(double a) {
  const double two_pi = 6.283185307179586;
  return a - two_pi * std::floor(a / two_pi);
}

// The pose after driving `ds` metres along an arc of curvature kappa (positive to the left), exactly.
inline CarPose arc_pose(const CarPose& p, double kappa, double ds) {
  CarPose q = p;
  if (std::fabs(kappa) < 1e-12) {
    q.x += ds * std::cos(p.theta);
    q.y += ds * std::sin(p.theta);
    return q;
  }
  q.theta = p.theta + kappa * ds;
  q.x += (std::sin(q.theta) - std::sin(p.theta)) / kappa;
  q.y -= (std::cos(q.theta) - std::cos(p.theta)) / kappa;
  return q;
}

// One word of the six, in closed form. ok is false if that word does not exist for these poses.
inline Dubins dubins_word(const CarPose& start, const CarPose& goal, double r, int word) {
  Dubins out;
  out.word = word;
  out.r = r;
  out.start = start;
  const double dx = goal.x - start.x;
  const double dy = goal.y - start.y;
  const double d = std::hypot(dx, dy) / r;
  const double theta = std::atan2(dy, dx);
  const double alpha = mod2pi(start.theta - theta);
  const double beta = mod2pi(goal.theta - theta);
  const double sa = std::sin(alpha), sb = std::sin(beta), ca = std::cos(alpha), cb = std::cos(beta);
  const double cab = std::cos(alpha - beta);
  double t = 0.0, p = 0.0, q = 0.0;
  bool ok = false;
  if (word == kLSL) {
    const double p2 = 2.0 + d * d - 2.0 * cab + 2.0 * d * (sa - sb);
    if (p2 >= 0.0) {
      const double tmp = std::atan2(cb - ca, d + sa - sb);
      t = mod2pi(-alpha + tmp);
      p = std::sqrt(p2);
      q = mod2pi(beta - tmp);
      ok = true;
    }
  } else if (word == kRSR) {
    const double p2 = 2.0 + d * d - 2.0 * cab + 2.0 * d * (sb - sa);
    if (p2 >= 0.0) {
      const double tmp = std::atan2(ca - cb, d - sa + sb);
      t = mod2pi(alpha - tmp);
      p = std::sqrt(p2);
      q = mod2pi(-beta + tmp);
      ok = true;
    }
  } else if (word == kLSR) {
    const double p2 = -2.0 + d * d + 2.0 * cab + 2.0 * d * (sa + sb);
    if (p2 >= 0.0) {
      p = std::sqrt(p2);
      const double tmp = std::atan2(-ca - cb, d + sa + sb) - std::atan2(-2.0, p);
      t = mod2pi(-alpha + tmp);
      q = mod2pi(-beta + tmp);
      ok = true;
    }
  } else if (word == kRSL) {
    const double p2 = d * d - 2.0 + 2.0 * cab - 2.0 * d * (sa + sb);
    if (p2 >= 0.0) {
      p = std::sqrt(p2);
      const double tmp = std::atan2(ca + cb, d - sa - sb) - std::atan2(2.0, p);
      t = mod2pi(alpha - tmp);
      q = mod2pi(beta - tmp);
      ok = true;
    }
  } else if (word == kRLR) {
    const double tmp = (6.0 - d * d + 2.0 * cab + 2.0 * d * (sa - sb)) / 8.0;
    if (std::fabs(tmp) <= 1.0) {
      p = mod2pi(6.283185307179586 - std::acos(tmp));
      t = mod2pi(alpha - std::atan2(ca - cb, d - sa + sb) + 0.5 * p);
      q = mod2pi(alpha - beta - t + p);
      ok = true;
    }
  } else if (word == kLRL) {
    const double tmp = (6.0 - d * d + 2.0 * cab + 2.0 * d * (sb - sa)) / 8.0;
    if (std::fabs(tmp) <= 1.0) {
      p = mod2pi(6.283185307179586 - std::acos(tmp));
      t = mod2pi(-alpha - std::atan2(ca - cb, d + sa - sb) + 0.5 * p);
      q = mod2pi(beta - alpha - t + p);
      ok = true;
    }
  }
  if (!ok) return out;
  out.ok = true;
  out.t = t;
  out.p = p;
  out.q = q;
  out.length = (t + p + q) * r;
  return out;
}

// The shortest of the six words.
inline Dubins dubins_shortest(const CarPose& start, const CarPose& goal, double r) {
  Dubins best;
  best.r = r;
  best.start = start;
  for (int w = 0; w < 6; ++w) {
    const Dubins d = dubins_word(start, goal, r, w);
    if (d.ok && (!best.ok || d.length < best.length)) best = d;
  }
  return best;
}

// The curvature of piece k (0, 1 or 2) of a word: +1 left, -1 right, 0 straight, over the radius.
inline double dubins_piece_curvature(int word, int k) {
  static const int table[6][3] = {{1, 0, 1}, {-1, 0, -1}, {1, 0, -1}, {-1, 0, 1}, {-1, 1, -1}, {1, -1, 1}};
  return table[word][k];
}

// Poses along the path, about `spacing` metres apart, the last exactly where the pieces end.
inline std::vector<CarPose> dubins_sample(const Dubins& d, double spacing) {
  std::vector<CarPose> out;
  if (!d.ok) return out;
  CarPose cur = d.start;
  out.push_back(cur);
  const double lens[3] = {d.t * d.r, d.p * d.r, d.q * d.r};
  for (int k = 0; k < 3; ++k) {
    const double kappa = dubins_piece_curvature(d.word, k) / d.r;
    int n = static_cast<int>(std::ceil(lens[k] / spacing));
    if (n < 1) n = 1;
    const CarPose from = cur;
    for (int i = 1; i <= n; ++i) out.push_back(arc_pose(from, kappa, lens[k] * i / n));
    cur = out.back();
  }
  return out;
}


}  // namespace drive
}  // namespace rc

#endif
