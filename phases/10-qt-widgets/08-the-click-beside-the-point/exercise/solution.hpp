#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <vector>
#include <rc/plot/series.hpp>

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
  // TODO: usable_across = across - 2 * margin, and the same down. dt = t1 - t0, dv = range.high - range.low. along = (time - t0) / dt, or 0.5 if dt is not above
  // 1e-12. up = (value - range.low) / dv, or 0.5 if dv is not above 1e-12. x = margin + along * usable_across. y = margin + (1 - up) * usable_down:
  // the screen's y goes DOWN while the value goes up.
  (void)v;
  (void)time;
  (void)value;
  return PixelPoint();
}

// The time and value at a pixel: the inverse of to_pixel. A usable area with no size maps to t0 and range.low.
inline DataPoint from_pixel(const ChartView& v, double x, double y) {
  // TODO: time = t0 + (x - margin) / usable_across * (t1 - t0), and value = range.low + (1 - (y - margin) / usable_down) * (range.high - range.low).
  // If a usable size is not above 1e-12, leave that part at t0 or range.low.
  (void)v;
  (void)x;
  (void)y;
  return DataPoint();
}

// The index of the sample whose drawn position is nearest the pixel (x, y), if it is within `tolerance` pixels (inclusive), otherwise -1. Samples outside
// the time shown are not on the chart and cannot be picked. A tie goes to the earlier sample.
inline int pick(const std::vector<Sample>& samples, const ChartView& v, double x, double y, double tolerance) {
  // TODO: for each sample whose time is inside [t0, t1], p = to_pixel(...); distance squared from (x, y). Keep the smallest that is at most tolerance squared;
  // the first of equal ones wins. Return its index, or -1. Do NOT compare time and value differences directly: they are different units.
  (void)samples;
  (void)v;
  (void)x;
  (void)y;
  (void)tolerance;
  return 0;
}

#endif  // LESSON_SOLUTION_HPP
