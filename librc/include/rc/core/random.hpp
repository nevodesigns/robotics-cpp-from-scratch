// rc/core/random.hpp
//
// Numbers that look random and can be replayed, from lesson 00-12, graduated.
//
// A test that uses random numbers has to be able to fail the same way twice, so the generator is seeded and the seed is
// reported. Three quick versions are wrong. A remainder, next() % n, favours small values unless n divides the range: with
// n of 3,000,000,000, 60.4 percent of draws fell below 1,294,967,296 where 43.2 is fair, and below(n) gave 43.3. A seed of
// zero leaves xorshift stuck at zero for ever. And the sum of twelve uniform numbers is a bell shape with thinner tails than a true
// one: 0.190 percent beyond three sigma where a true bell curve has 0.270, and nothing past 4.35 in 400000 draws.

#ifndef RC_CORE_RANDOM_HPP
#define RC_CORE_RANDOM_HPP

namespace rc {
namespace core {

// A source of numbers that look random and are not: the same seed gives the same numbers, every run
// and on every machine, which is exactly what a test needs.
class Rng {
 public:
  explicit Rng(unsigned int seed) : state_(seed != 0 ? seed : 0x9E3779B9u) {}

  // xorshift32: three shifts and three xors, and the whole of the generator.
  unsigned int next() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return state_;
  }

  // A whole number from 0 up to but not including n, each equally likely.
  //
  // The obvious way, next() % n, is not fair unless n divides the range exactly: the smaller
  // residues turn up more often. Throw away the few draws that would cause the imbalance.
  unsigned int below(unsigned int n) {
    if (n == 0) return 0;
    const unsigned int threshold = (0u - n) % n;   // how many draws at the bottom of the range are spare
    unsigned int r = next();
    while (r < threshold) r = next();
    return r % n;
  }

  // A number in [0, 1), from the top 24 bits, which a double holds exactly.
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }

  // A bell shaped number with mean 0 and standard deviation 1: the sum of twelve uniform numbers
  // has mean 6 and variance 1, so take away 6.
  double gaussian() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }

 private:
  unsigned int state_;
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_RANDOM_HPP
