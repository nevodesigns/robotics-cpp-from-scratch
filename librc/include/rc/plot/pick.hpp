// rc/plot/pick.hpp
//
// The click beside the point, from lesson 10-08, graduated.
//
// With 100 seconds across 1000 pixels and 1 volt up 100, a click at the time of one sample and the value of another was 40 pixels from the first and 9 from the second: the second is the
// one under the cursor, and the nearest in raw seconds and volts is the first. The inverse map recovers every point it drew to 1e-9.

#ifndef RC_PLOT_PICK_HPP
#define RC_PLOT_PICK_HPP

#include <cstddef>
#include <vector>
#include <rc/plot/series.hpp>

namespace rc {
namespace plot {

// A chart is a map from data (seconds, volts) to pixels, and a click on it is a pixel that has to be mapped back. Two traps are in the way. The map has to be
// the exact inverse of the one that drew the chart, y going down the screen while the value goes up. And "the nearest sample" has to be nearest in PIXELS: a
// chart with 100 seconds across and 1 volt up has nothing in common between a second and a volt, and a distance that adds them up picks the wrong sample.

using rc::plot::Range;
using rc::plot::Sample;

// What the chart shows and how big it is on screen. The usable area is `margin` in from each edge.
struct ChartView {
  double t0 = 0.0;
  double t1 = 1.0;
  Range range{0.0, 1.0};
  double across = 100.0;    // pixels
  double down = 100.0;
  double margin = 0.0;
};

struct PixelPoint {
  double x = 0.0;
  double y = 0.0;
};

struct DataPoint {
  double time = 0.0;
  double value = 0.0;
};

// Where a time and a value are drawn: t0 at the left edge of the usable area, t1 at the right, range.low at the bottom and range.high at the top. A view
// with no width in time, or no height in value, puts the point in the middle of that direction.
inline PixelPoint to_pixel(const ChartView& v, double time, double value) {
  const double usable_across = v.across - 2.0 * v.margin;
  const double usable_down = v.down - 2.0 * v.margin;
  const double dt = v.t1 - v.t0;
  const double dv = v.range.high - v.range.low;
  const double along = dt > 1e-12 ? (time - v.t0) / dt : 0.5;
  const double up = dv > 1e-12 ? (value - v.range.low) / dv : 0.5;
  PixelPoint p;
  p.x = v.margin + along * usable_across;
  p.y = v.margin + (1.0 - up) * usable_down;
  return p;
}

// The time and value at a pixel: the inverse of to_pixel. A usable area with no size maps to t0 and range.low.
inline DataPoint from_pixel(const ChartView& v, double x, double y) {
  const double usable_across = v.across - 2.0 * v.margin;
  const double usable_down = v.down - 2.0 * v.margin;
  DataPoint d;
  d.time = v.t0;
  d.value = v.range.low;
  if (usable_across > 1e-12) d.time = v.t0 + (x - v.margin) / usable_across * (v.t1 - v.t0);
  if (usable_down > 1e-12) d.value = v.range.low + (1.0 - (y - v.margin) / usable_down) * (v.range.high - v.range.low);
  return d;
}

// The index of the sample whose drawn position is nearest the pixel (x, y), if it is within `tolerance` pixels (inclusive), otherwise -1. Samples outside
// the time shown are not on the chart and cannot be picked. A tie goes to the earlier sample.
inline int pick(const std::vector<Sample>& samples, const ChartView& v, double x, double y, double tolerance) {
  int best = -1;
  double best_distance = 0.0;
  const double limit = tolerance * tolerance;
  for (std::size_t i = 0; i < samples.size(); ++i) {
    const Sample& s = samples[i];
    if (s.time < v.t0 || s.time > v.t1) continue;
    const PixelPoint p = to_pixel(v, s.time, s.value);
    const double dx = p.x - x;
    const double dy = p.y - y;
    const double distance = dx * dx + dy * dy;
    if (distance > limit) continue;
    if (best < 0 || distance < best_distance) {
      best = static_cast<int>(i);
      best_distance = distance;
    }
  }
  return best;
}
}  // namespace plot
}  // namespace rc

#endif  // RC_PLOT_PICK_HPP
