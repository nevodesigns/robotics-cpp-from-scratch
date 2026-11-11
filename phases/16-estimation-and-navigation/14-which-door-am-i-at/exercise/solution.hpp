#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

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
    // TODO: sum the numbers. If the sum is not above zero, or is not finite (std::isfinite), every place gets
    // 1 / size, and stop. Otherwise divide every number by the sum.
  }

  // The robot moved `shift` places round the ring. It meant to move exactly that far, and managed it with probability `p_exact`;
  // it fell one place short with probability `p_short` and went one place too far with `p_long`. Every place's belief is moved on by
  // the three amounts, each scaled by its probability, and the ring wraps.
  void predict(int shift, double p_exact, double p_short, double p_long) {
    // TODO: a new vector of zeros. For each place i (0 to n - 1), with n the number of places:
    // exact = (i + shift) wrapped into 0..n-1, shorter = (i + shift - 1) wrapped, longer = (i + shift + 1) wrapped.
    // Wrapping a possibly negative number: ((v % n) + n) % n.
    // next[exact] += p_[i] * p_exact; next[shorter] += p_[i] * p_short; next[longer] += p_[i] * p_long.
    // Then p_ = next and normalise().
    (void)shift;
    (void)p_exact;
    (void)p_short;
    (void)p_long;
  }

  // The sensor read `reading` (1 for a door, 0 for a wall). A place whose map entry matches is `p_match` times as believable as before,
  // one that does not is `p_mismatch` times as believable. Then normalise.
  void correct(const std::vector<int>& world, int reading, double p_match, double p_mismatch) {
    // TODO: for each place i that has a world entry, multiply p_[i] by p_match if world[i] == reading, otherwise by p_mismatch.
    // Then normalise().
    (void)world;
    (void)reading;
    (void)p_match;
    (void)p_mismatch;
  }

  // The place with the most belief; the lowest numbered if several share it.
  std::size_t most_likely() const {
    // TODO: the index of the largest number; the lowest index if several share it.
    return std::size_t(0);
  }

  double peak() const { return p_[most_likely()]; }

  // How unsure the belief is, in bits: zero when one place has all of it, log2 of the number of places when it is spread evenly.
  double entropy() const {
    // TODO: minus the sum of v * log2(v) over the places with v above zero.
    return -1.0;
  }

 private:
  std::vector<double> p_;
};

#endif  // LESSON_SOLUTION_HPP
