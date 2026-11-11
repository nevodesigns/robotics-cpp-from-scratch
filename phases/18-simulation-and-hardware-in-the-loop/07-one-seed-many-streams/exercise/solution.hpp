#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstdint>
#include <map>
#include <string>

// One step of the splitmix64 mixer: a way to turn any 64 bit number into one that looks unrelated to it, so that seeds close together are far apart.
inline std::uint64_t mix64(std::uint64_t z) {
  // TODO: splitmix64: z += 0x9E3779B97F4A7C15; z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9; z = (z ^ (z >> 27)) * 0x94D049BB133111EB; return z ^ (z >> 31). All unsigned 64 bit
  (void)z;
  return 0;
}

// The seed for the part of the simulation called `name`, from the master seed: the name is hashed (FNV-1a, 64 bit) and the result mixed with the master.
// The same master and name always give the same seed, whatever else exists, and names that differ by one letter give seeds with nothing in common.
inline std::uint64_t derive_seed(std::uint64_t master, const std::string& name) {
  // TODO: FNV-1a 64 bit over the characters of the name: h = 14695981039346656037, for each character h ^= the byte, h *= 1099511628211. The seed is mix64(mix64(master) ^ h)
  (void)master;
  (void)name;
  return 0;
}

// A source of random numbers for one part of a simulation.
class Stream {
 public:
  explicit Stream(std::uint64_t seed) : state_(seed) {}

  std::uint64_t next() {
    // TODO: add 0x9E3779B97F4A7C15 to state_, then the same mixing as mix64 on that new state: z ^ (z >> 30) times 0xBF58476D1CE4E5B9, then z ^ (z >> 27) times 0x94D049BB133111EB, then z ^ (z >> 31)
    (void)state_;
    return 0;
  }

  // A number from 0 up to but not including 1, with 53 bits.
  double uniform() { return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0); }

  // Roughly normal, mean 0 and standard deviation 1: twelve uniform numbers added, minus six.
  double gaussian() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }

 private:
  std::uint64_t state_ = 0;
};

// All the streams of one simulation. Ask for a stream by the name of the part that uses it: each name has its own, made the first time it is asked
// for and the same one every time after, and its numbers do not depend on which other names there are or in what order they were asked for.
class Streams {
 public:
  explicit Streams(std::uint64_t master_seed) : master_(master_seed) {}

  Stream& get(const std::string& name) {
    // TODO: find the name in streams_; if it is not there, insert Stream(derive_seed(master_, name)) under it. Return the stream, always the same one for the same name
    (void)name;
    (void)master_;
    static Stream placeholder(0);        // so that this compiles and runs before it is written
    return placeholder;
  }

  std::size_t count() const { return streams_.size(); }

 private:
  std::uint64_t master_ = 0;
  std::map<std::string, Stream> streams_;
};

#endif  // LESSON_SOLUTION_HPP
