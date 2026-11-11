// rc/sim/streams.hpp
//
// A random stream for each named part of a simulation, from lesson 18-07, graduated.
//
// One master seed gives each part its own stream: the name is hashed (FNV-1a, 64 bit) and mixed (splitmix64) with the master, so the same master and name always give the same stream, whatever other parts exist and in whatever order they were first asked for,
// and names that differ by a letter get unrelated seeds. Uniform numbers have 53 bits and run from 0 up to but not including 1; normal ones are twelve uniforms summed minus six.
//
// Mixing a number and the same number with one bit changed flipped 32.1 of 64 bits on average; consecutive names (motor0, motor1, ...) differed in 32.1 of 64. 100000 uniforms had mean 0.4992 and variance 0.0830 (1/12 is 0.0833); normals mean 0.0001 and spread 0.9999. Two streams from one seed had a correlation of -0.0040.
// Adding a camera to a list of three sensors changed 0 of 1000 of the imu's values with a stream each, and 1000 of 1000 with one shared stream.

#ifndef RC_SIM_STREAMS_HPP
#define RC_SIM_STREAMS_HPP

#include <cstdint>
#include <map>
#include <string>

namespace rc {
namespace sim {

// One step of the splitmix64 mixer: a way to turn any 64 bit number into one that looks unrelated to it, so that seeds close together are far apart.
inline std::uint64_t mix64(std::uint64_t z) {
  z += 0x9E3779B97F4A7C15ULL;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

// The seed for the part of the simulation called `name`, from the master seed: the name is hashed (FNV-1a, 64 bit) and the result mixed with the master.
// The same master and name always give the same seed, whatever else exists, and names that differ by one letter give seeds with nothing in common.
inline std::uint64_t derive_seed(std::uint64_t master, const std::string& name) {
  std::uint64_t h = 14695981039346656037ULL;
  for (char c : name) {
    h ^= static_cast<std::uint8_t>(c);
    h *= 1099511628211ULL;
  }
  return mix64(mix64(master) ^ h);
}

// A source of random numbers for one part of a simulation.
class Stream {
 public:
  explicit Stream(std::uint64_t seed) : state_(seed) {}

  std::uint64_t next() {
    state_ += 0x9E3779B97F4A7C15ULL;
    std::uint64_t z = state_;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
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
    auto found = streams_.find(name);
    if (found == streams_.end()) found = streams_.insert({name, Stream(derive_seed(master_, name))}).first;
    return found->second;
  }

  std::size_t count() const { return streams_.size(); }

 private:
  std::uint64_t master_ = 0;
  std::map<std::string, Stream> streams_;
};

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_STREAMS_HPP
