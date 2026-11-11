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
  if (stream.empty() || t < stream.front().t) return false;
  const auto after = std::upper_bound(stream.begin(), stream.end(), t,
                                      [](double time, const Sample& s) { return time < s.t; });
  out = (after - 1)->value;
  return true;
}

// The stream's value at `t`, found by joining the two samples either side of it with a straight line.
// False, with `out` untouched, if `t` is before the first sample or after the last, or if the two samples
// around it are more than `max_gap` seconds apart: that is a hole, and the line across it is made up.
inline bool interpolate_at(const std::vector<Sample>& stream, double t, double max_gap, double& out) {
  if (stream.empty() || t < stream.front().t || t > stream.back().t) return false;
  const auto after = std::upper_bound(stream.begin(), stream.end(), t,
                                      [](double time, const Sample& s) { return time < s.t; });
  if (after == stream.end()) {   // exactly the last sample
    out = stream.back().value;
    return true;
  }
  const Sample& b = *after;
  const Sample& a = *(after - 1);
  if (a.t == t) {
    out = a.value;
    return true;
  }
  const double gap = b.t - a.t;
  if (gap <= 0.0 || gap > max_gap) return false;
  out = a.value + (b.value - a.value) * (t - a.t) / gap;
  return true;
}

// The stream at each of `times`, leaving out the instants it cannot answer for. Each result carries its own instant.
inline std::vector<Sample> resample(const std::vector<Sample>& stream, const std::vector<double>& times, double max_gap) {
  std::vector<Sample> out;
  for (double t : times) {
    double v = 0.0;
    if (interpolate_at(stream, t, max_gap, v)) {
      Sample s;
      s.t = t;
      s.value = v;
      out.push_back(s);
    }
  }
  return out;
}

#endif  // LESSON_SOLUTION_HPP
