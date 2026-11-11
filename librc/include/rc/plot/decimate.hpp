// rc/plot/decimate.hpp
//
// Decimation that keeps the spike, from lesson 10-06, graduated.
//
// A million samples with one spike of 5 not on a multiple of 1250, drawn as 800 columns: every 1250th sample never saw the spike and the lowest-and-highest of each column did, in at most 1600 points.

#ifndef RC_PLOT_DECIMATE_HPP
#define RC_PLOT_DECIMATE_HPP

#include <cstddef>
#include <vector>
#include <rc/plot/series.hpp>

namespace rc {
namespace plot {

// A chart 800 pixels wide cannot show a million samples, and drawing a million line segments to find that out is slow. The usual answer is to
// draw every 1250th sample, and it loses the one sample that mattered: a spike that is not on a multiple of 1250 is never drawn.
//
// The answer that keeps the shape is to cut the time axis into one bin per pixel column and keep, for each, the lowest and highest value that fell in
// it. A column is then a vertical line from its low to its high, a spike is a column that is tall, and the work is at most two points per column.

using rc::plot::Sample;

struct Column {
  int n = 0;            // samples that fell in this column
  double low = 0.0;     // valid only when n > 0
  double high = 0.0;
};

// One Column for each of `columns` equal bins of time from t0 to t1. A sample belongs to the bin its time falls in; a sample at exactly t1 belongs to
// the last bin; a sample outside the range, or a column count below 1, is not counted. Samples need not be in order.
inline std::vector<Column> decimate(const std::vector<Sample>& samples, double t0, double t1, int columns) {
  std::vector<Column> out(columns > 0 ? static_cast<std::size_t>(columns) : 0);
  if (columns < 1 || !(t1 > t0)) return out;
  const double width = (t1 - t0) / static_cast<double>(columns);
  for (const Sample& s : samples) {
    if (s.time < t0 || s.time > t1) continue;
    int c = static_cast<int>((s.time - t0) / width);
    if (c >= columns) c = columns - 1;
    Column& col = out[static_cast<std::size_t>(c)];
    if (col.n == 0) {
      col.low = col.high = s.value;
    } else {
      if (s.value < col.low) col.low = s.value;
      if (s.value > col.high) col.high = s.value;
    }
    ++col.n;
  }
  return out;
}

// How many points it takes to draw these columns: two for a column whose samples differ (its low and its high), one for a column of equal values, none for an empty one.
inline std::size_t points_to_draw(const std::vector<Column>& columns) {
  std::size_t points = 0;
  for (const Column& c : columns) {
    if (c.n == 0) continue;
    points += c.low == c.high ? 1 : 2;
  }
  return points;
}
}  // namespace plot
}  // namespace rc

#endif  // RC_PLOT_DECIMATE_HPP
