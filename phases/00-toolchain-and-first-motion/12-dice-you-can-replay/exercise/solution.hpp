#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// A source of numbers that look random and are not: the same seed gives the same numbers, every run
// and on every machine, which is exactly what a test needs.
class Rng {
 public:
  explicit Rng(unsigned int seed) : state_(seed) {
    // TODO: a seed of zero breaks xorshift, which then returns zero for ever. Use 0x9E3779B9u
    // instead when the seed is zero.
  }

  unsigned int next() {
    // TODO: xorshift32. state_ ^= state_ << 13; state_ ^= state_ >> 17; state_ ^= state_ << 5;
    // then return state_.
    (void)state_;
    return 0;
  }

  // A whole number from 0 up to but not including n, each equally likely.
  unsigned int below(unsigned int n) {
    // TODO: n of zero gives 0. Otherwise threshold = (0u - n) % n, draw with next() until a draw
    // is not below the threshold, and return that draw % n. The draws below the threshold are the ones
    // that would make the small residues too likely.
    (void)n;
    return 0;
  }

  // A number in [0, 1), from the top 24 bits: (next() >> 8) / 16777216.0.
  double uniform() { return 0.0; }

  // Mean 0, standard deviation 1: the sum of twelve uniform() numbers, minus 6.
  double gaussian() { return 0.0; }

 private:
  unsigned int state_;
};

#endif  // LESSON_SOLUTION_HPP
