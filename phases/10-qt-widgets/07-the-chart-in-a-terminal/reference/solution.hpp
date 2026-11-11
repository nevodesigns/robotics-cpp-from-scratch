#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <string>
#include <vector>
#include <rc/plot/decimate.hpp>
#include <rc/plot/series.hpp>
#include <rc/plot/ticks.hpp>

// The chart of a signal as lines of text, which is what a robot on a network with no screen has. It is made of the three pieces before it: columns
// of lowest and highest values (10-06), a range with room around it (10-02), and ticks on nice numbers (10-05).
//
// The range is where a chart lies. A signal that is steady has a span of nothing, and a chart that scales the span to the screen draws the noise
// in its last digit as a storm. The range here is never narrower than `min_span`: a steady signal is a flat line.

using rc::plot::at_least;
using rc::plot::Column;
using rc::plot::decimate;
using rc::plot::padded;
using rc::plot::Range;
using rc::plot::Sample;
using rc::plot::tick_label;
using rc::plot::Ticks;
using rc::plot::ticks_for;

// The range of values to show: from the lowest low to the highest high of the columns that have samples, with 5 percent of room above and below,
// and never narrower than min_span, which is centred on the middle if the data is narrower. With no samples, 0 to 1.
inline Range value_range(const std::vector<Column>& columns, double min_span) {
  bool any = false;
  Range range{0.0, 1.0};
  for (const Column& c : columns) {
    if (c.n == 0) continue;
    if (!any) {
      range.low = c.low;
      range.high = c.high;
      any = true;
      continue;
    }
    if (c.low < range.low) range.low = c.low;
    if (c.high > range.high) range.high = c.high;
  }
  if (!any) return range;
  return at_least(padded(range, 0.05), min_span);
}

// The row, counted from the top of a chart of `rows` rows, that a value is on: the top row is range.high and the bottom row is range.low, rounded to
// the nearest row and kept inside the chart. A range with no height puts everything in the middle row.
inline int row_for(double value, const Range& range, int rows) {
  const double height = range.high - range.low;
  if (rows < 1) return 0;
  if (!(height > 0.0)) return rows / 2;
  int row = static_cast<int>(std::lround((range.high - value) / height * static_cast<double>(rows - 1)));
  if (row < 0) row = 0;
  if (row > rows - 1) row = rows - 1;
  return row;
}

// The chart as `rows` lines of text. Each line is a label (or spaces), " |", and `columns` characters, with a '*' wherever a column has a sample in that
// row, from its lowest value to its highest. Labels are right aligned to the widest, and are on the rows of the ticks. No lines if columns < 1 or rows < 2.
inline std::vector<std::string> render_text(const std::vector<Sample>& samples, double t0, double t1, int columns, int rows, double min_span) {
  std::vector<std::string> out;
  if (columns < 1 || rows < 2) return out;

  const std::vector<Column> cols = decimate(samples, t0, t1, columns);
  const Range range = value_range(cols, min_span);

  std::vector<std::string> grid(static_cast<std::size_t>(rows), std::string(static_cast<std::size_t>(columns), ' '));
  for (int c = 0; c < columns; ++c) {
    const Column& col = cols[static_cast<std::size_t>(c)];
    if (col.n == 0) continue;
    const int top = row_for(col.high, range, rows);
    const int bottom = row_for(col.low, range, rows);
    for (int r = top; r <= bottom; ++r) grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = '*';
  }

  const Ticks ticks = ticks_for(range.low, range.high, rows / 3 >= 2 ? rows / 3 : 2);
  std::vector<std::string> labels(static_cast<std::size_t>(rows));
  std::size_t width = 0;
  for (int k = 0; k < ticks.count; ++k) {
    const double v = ticks.value(k);
    const int r = row_for(v, range, rows);
    labels[static_cast<std::size_t>(r)] = tick_label(v, ticks.decimals);
  }
  for (const std::string& l : labels)
    if (l.size() > width) width = l.size();

  for (int r = 0; r < rows; ++r) {
    const std::string& l = labels[static_cast<std::size_t>(r)];
    out.push_back(std::string(width - l.size(), ' ') + l + " |" + grid[static_cast<std::size_t>(r)]);
  }
  return out;
}

#endif  // LESSON_SOLUTION_HPP
