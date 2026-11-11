#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <vector>
#include <rc/plot/series.hpp>

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
  // TODO: out = a vector of `columns` Columns (none if columns < 1). If columns < 1 or t1 is not more than t0, return it. width = (t1 - t0) / columns.
  // For each sample inside [t0, t1]: c = int((time - t0) / width), at most columns - 1. If that column's n is 0 set low and high to the value, otherwise
  // lower low or raise high. Then ++n.
  (void)samples;
  (void)t0;
  (void)t1;
  return std::vector<Column>(columns > 0 ? static_cast<std::size_t>(columns) : 0);
}

// How many points it takes to draw these columns: two for a column whose samples differ (its low and its high), one for a column of equal values, none for an empty one.
inline std::size_t points_to_draw(const std::vector<Column>& columns) {
  // TODO: for each column with n > 0, add 2 if low != high, otherwise 1.
  (void)columns;
  return 999999;
}

#endif  // LESSON_SOLUTION_HPP
