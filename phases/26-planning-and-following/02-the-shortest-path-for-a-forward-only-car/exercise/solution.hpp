#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/drive/hybrid_astar.hpp>
#include <vector>

using namespace rc::drive;

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
  (void)sa;  // TODO: for each of the six words, work out the middle piece p from the geometry (two circles of radius 1 and the line between them), then the arcs t and q from the headings; a word does not exist if the square root or the arc cosine has no answer
  (void)sb;
  (void)ca;
  (void)cb;
  (void)cab;
  (void)d;
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


#endif
