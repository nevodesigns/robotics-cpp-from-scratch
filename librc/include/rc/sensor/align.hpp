// rc/sensor/align.hpp
//
// Streams on the same instants, from lesson 15-15, graduated.
//
// Two sensors tick at their own rates. To compare or fuse them, one has to be read at the other's instants, and the easy way, the newest value
// it has sent, is late by half a sample on average. A sine of amplitude 1 at 1 Hz sampled at 10 Hz and read at 100 instants a second was 0.2384 rms off
// when held and 0.0253 off when the two samples either side were joined by a line, against 0.2565 and 0.0255 from the maths (hold: w/sqrt(2) x h/sqrt(3); line: w squared/sqrt(2) x h squared/(2 sqrt(30))).
//
// Two streams reading the same signal exactly disagreed by 0.2384 rms (worst 0.5878) with the slow one held and 0.0253 with it interpolated.
// No value is made up before the first sample, after the last, or across a gap longer than max_gap: with a hole from 1.0 s to 1.6 s,
// 192 of 251 instants were answered and none of the 59 inside the hole.

#ifndef RC_SENSOR_ALIGN_HPP
#define RC_SENSOR_ALIGN_HPP

#include <algorithm>
#include <cstddef>
#include <vector>

namespace rc {
namespace sensor {

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

}  // namespace sensor
}  // namespace rc

#endif  // RC_SENSOR_ALIGN_HPP
