#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cstddef>
#include <vector>

// One reading and the instant it describes, in seconds. Streams are sorted by time.
struct Sample {
  double t = 0.0;
  double value = 0.0;
};

// The newest sample at or before `t`: what a loop does if it just uses the last value it was given.
// False if the stream has nothing that old.
inline bool hold_at(const std::vector<Sample>& stream, double t, double& out) {
  // TODO: false for an empty stream or a t before the first sample. Otherwise find the first sample AFTER t
  // (std::upper_bound with a comparison of t against s.t does it), step back one, and give its value.
  (void)stream;
  (void)t;
  (void)out;
  return false;
}

// The stream's value at `t`, found by joining the two samples either side of it with a straight line.
// False, with `out` untouched, if `t` is before the first sample or after the last, or if the two samples
// around it are more than `max_gap` seconds apart: that is a hole, and the line across it is made up.
inline bool interpolate_at(const std::vector<Sample>& stream, double t, double max_gap, double& out) {
  // TODO: false for an empty stream, t before the first or after the last sample.
  // Find the first sample after t. If there is none, t is exactly the last sample: give its value.
  // Let a be the sample before it and b the one after. If a.t == t give a.value.
  // If b.t - a.t is zero, negative or more than max_gap, return false.
  // Otherwise a.value + (b.value - a.value) * (t - a.t) / (b.t - a.t).
  (void)stream;
  (void)t;
  (void)max_gap;
  (void)out;
  return false;
}

// The stream at each of `times`, leaving out the instants it cannot answer for. Each result carries its own instant.
inline std::vector<Sample> resample(const std::vector<Sample>& stream, const std::vector<double>& times, double max_gap) {
  // TODO: for each time, interpolate_at(); keep a Sample{t, value} for each one that succeeds.
  (void)stream;
  (void)times;
  (void)max_gap;
  return std::vector<Sample>();
}

#endif  // LESSON_SOLUTION_HPP
