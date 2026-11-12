#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/field/frames.hpp>
#include <utility>
#include <vector>

using P2 = std::pair<double, double>;

struct IcpResult {
  rc::field::Tf2 move;   // takes a source point to its place in the target
  double rms = 0.0;      // the root mean square distance of the pairs after the last move
  int rounds = 0;
  int paired = 0;
  bool converged = false;
};

inline P2 move_point(const rc::field::Tf2& t, const P2& p) {
  const double c = std::cos(t.theta), s = std::sin(t.theta);
  return P2(t.x + c * p.first - s * p.second, t.y + s * p.first + c * p.second);
}

// Matches source to target. `guess` is the first idea of the move; pairs further apart than max_pair_distance are not used; it stops when a round moves less than tol (metres and radians) or after max_rounds.
inline IcpResult icp(const std::vector<P2>& source, const std::vector<P2>& target, const rc::field::Tf2& guess, int max_rounds, double max_pair_distance, double tol) {
  IcpResult r;
  r.move = guess;
  if (source.empty() || target.empty()) return r;
  for (int round = 1; round <= max_rounds; ++round) {
    std::vector<P2> src, dst;
    for (std::size_t i = 0; i < source.size(); ++i) {
      const P2 p = move_point(r.move, source[i]);
      double best = max_pair_distance;
      std::size_t bj = target.size();
      for (std::size_t j = 0; j < target.size(); ++j) {
        const double d = std::hypot(p.first - target[j].first, p.second - target[j].second);
        if (d <= best) {
          best = d;
          bj = j;
        }
      }
      if (bj < target.size()) {
        src.push_back(p);
        dst.push_back(target[bj]);
      }
    }
    r.paired = static_cast<int>(src.size());
    r.rounds = round;
    if (src.size() < 3) return r;
    const double n = static_cast<double>(src.size());
    (void)dst;  // TODO: centre the paired points on their means; the rotation that best aligns them is atan2(sum of cross, sum of dot) of the centred pairs; the translation takes the rotated source mean to the target mean
    rc::field::Tf2 inc;
    r.move = rc::field::compose(inc, r.move);
    double sq = 0.0;
    for (std::size_t i = 0; i < src.size(); ++i) {
      const P2 q = move_point(inc, src[i]);
      sq += (q.first - dst[i].first) * (q.first - dst[i].first) + (q.second - dst[i].second) * (q.second - dst[i].second);
    }
    r.rms = std::sqrt(sq / n);
    if (std::fabs(inc.x) < tol && std::fabs(inc.y) < tol && std::fabs(inc.theta) < tol) {
      r.converged = true;
      return r;
    }
  }
  return r;
}


#endif
