// rc/nav/belief.hpp
//
// A histogram belief over places on a ring, from lesson 16-14, graduated.
//
// A belief is a number for each place, summing to 1. Moving shifts it round the ring and smears it by the slip (all at place 5, moved one place with 0.8 / 0.1 / 0.1: 0.80 at 6, 0.10 at 5 and 7);
// sensing multiplies each place by how believable the reading makes it and normalises. A belief with nothing left in it goes back to knowing nothing.
//
// One reading of a door in a 24 place corridor with 9 doors left 9 places with a share near the top, each door 0.0938 and each wall 0.0104, entropy 3.91 bits from 4.58: a belief that can be at several
// places at once, which a single number cannot say. Walking and sensing, the share at the true place went 0.094, 0.286, 0.611, 0.794 and 0.799 at steps 0, 4, 7, 12 and 14.
//
// A belief told the sensor never errs, given one wrong reading after 25 steps, kept 0.042 of the true place and took 5 steps to get above a half; allowing a wrong reading probability 0.01 or 0.1 it kept 1.000.

#ifndef RC_NAV_BELIEF_HPP
#define RC_NAV_BELIEF_HPP

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace nav {

// A belief about which of n places on a ring the robot is at: a number for each, summing to 1.
class Belief {
 public:
  // Starts knowing nothing: every place equally likely.
  explicit Belief(std::size_t n) : p_(n > 0 ? n : 1, 1.0 / static_cast<double>(n > 0 ? n : 1)) {}

  std::size_t size() const { return p_.size(); }
  double at(std::size_t i) const { return i < p_.size() ? p_[i] : 0.0; }
  const std::vector<double>& cells() const { return p_; }

  // Make the numbers sum to 1. If they sum to zero, or to something that is not a number, nothing is left to believe in,
  // and the belief goes back to knowing nothing.
  void normalise() {
    double sum = 0.0;
    for (double v : p_) sum += v;
    if (!(sum > 0.0) || !std::isfinite(sum)) {
      for (double& v : p_) v = 1.0 / static_cast<double>(p_.size());
      return;
    }
    for (double& v : p_) v /= sum;
  }

  // The robot moved `shift` places round the ring. It meant to move exactly that far, and managed it with probability `p_exact`;
  // it fell one place short with probability `p_short` and went one place too far with `p_long`. Every place's belief is moved on by
  // the three amounts, each scaled by its probability, and the ring wraps.
  void predict(int shift, double p_exact, double p_short, double p_long) {
    const int n = static_cast<int>(p_.size());
    std::vector<double> next(p_.size(), 0.0);
    for (int i = 0; i < n; ++i) {
      const int exact = ((i + shift) % n + n) % n;
      const int shorter = ((i + shift - 1) % n + n) % n;
      const int longer = ((i + shift + 1) % n + n) % n;
      next[static_cast<std::size_t>(exact)] += p_[static_cast<std::size_t>(i)] * p_exact;
      next[static_cast<std::size_t>(shorter)] += p_[static_cast<std::size_t>(i)] * p_short;
      next[static_cast<std::size_t>(longer)] += p_[static_cast<std::size_t>(i)] * p_long;
    }
    p_ = next;
    normalise();
  }

  // The sensor read `reading` (1 for a door, 0 for a wall). A place whose map entry matches is `p_match` times as believable as before,
  // one that does not is `p_mismatch` times as believable. Then normalise.
  void correct(const std::vector<int>& world, int reading, double p_match, double p_mismatch) {
    for (std::size_t i = 0; i < p_.size() && i < world.size(); ++i) p_[i] *= (world[i] == reading) ? p_match : p_mismatch;
    normalise();
  }

  // The place with the most belief; the lowest numbered if several share it.
  std::size_t most_likely() const {
    std::size_t best = 0;
    for (std::size_t i = 1; i < p_.size(); ++i)
      if (p_[i] > p_[best]) best = i;
    return best;
  }

  double peak() const { return p_[most_likely()]; }

  // How unsure the belief is, in bits: zero when one place has all of it, log2 of the number of places when it is spread evenly.
  double entropy() const {
    double h = 0.0;
    for (double v : p_)
      if (v > 0.0) h -= v * std::log2(v);
    return h;
  }

 private:
  std::vector<double> p_;
};

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_BELIEF_HPP
