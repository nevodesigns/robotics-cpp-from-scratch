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
  // TODO: over the columns with n > 0, the lowest low and the highest high. If there are none, return Range{0.0, 1.0}. Otherwise
  // at_least(padded(range, 0.05), min_span): padded adds a fraction of the span on each side, at_least widens a range that is narrower than min_span.
  (void)columns;
  (void)min_span;
  return Range{0.0, 1.0};
}

// The row, counted from the top of a chart of `rows` rows, that a value is on: the top row is range.high and the bottom row is range.low, rounded to
// the nearest row and kept inside the chart. A range with no height puts everything in the middle row.
inline int row_for(double value, const Range& range, int rows) {
  // TODO: height = high - low. If rows < 1 return 0. If height is not above 0 return rows / 2. Otherwise
  // row = lround((high - value) / height * (rows - 1)), kept between 0 and rows - 1.
  (void)value;
  (void)range;
  (void)rows;
  return 0;
}

// The chart as `rows` lines of text. Each line is a label (or spaces), " |", and `columns` characters, with a '*' wherever a column has a sample in that
// row, from its lowest value to its highest. Labels are right aligned to the widest, and are on the rows of the ticks. No lines if columns < 1 or rows < 2.
inline std::vector<std::string> render_text(const std::vector<Sample>& samples, double t0, double t1, int columns, int rows, double min_span) {
  // TODO: no lines if columns < 1 or rows < 2. cols = decimate(samples, t0, t1, columns); range = value_range(cols, min_span).
  // A grid of `rows` strings of `columns` spaces; for each column with n > 0 put '*' in every row from row_for(high) down to row_for(low).
  // ticks = ticks_for(range.low, range.high, rows / 3, at least 2); for each tick, labels[row_for(value)] = tick_label(value, ticks.decimals).
  // Each output line is the label right aligned to the widest label, then " |", then the grid row.
  (void)samples;
  (void)t0;
  (void)t1;
  (void)columns;
  (void)rows;
  (void)min_span;
  return std::vector<std::string>();
}

#endif  // LESSON_SOLUTION_HPP
