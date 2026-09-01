#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

constexpr double kWindow = 10.0;
constexpr std::size_t kCapacity = 500;

// The drawing, supplied. As in lesson 00-07, the arithmetic above it is the
// part that is wrong when a chart is wrong.
std::string draw(const Series& series, const Range& range, int columns, int rows) {
  if (series.empty()) return "  (no samples)\n";

  std::vector<std::string> grid(static_cast<std::size_t>(rows),
                                std::string(static_cast<std::size_t>(columns), ' '));

  for (std::size_t i = 0; i < series.size(); ++i) {
    const Point point = place_sample(series.at(i), series.newest_time(), series.window(),
                                     range, static_cast<double>(columns),
                                     static_cast<double>(rows), 0.0);
    const int column = static_cast<int>(point.across);
    const int row = static_cast<int>(point.down);
    if (column < 0 || column >= columns || row < 0 || row >= rows) continue;
    grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] = '#';
  }

  std::ostringstream out;
  for (int r = 0; r < rows; ++r) {
    const double fraction = 1.0 - static_cast<double>(r) / static_cast<double>(rows - 1);
    out << "  " << std::setw(9) << std::fixed << std::setprecision(4)
        << range.low + fraction * span(range) << " |" << grid[static_cast<std::size_t>(r)] << "\n";
  }
  return out.str();
}

// How many rows of a chart the signal actually occupies.
int rows_used(const Series& series, const Range& range, int columns, int rows) {
  int top = rows, bottom = -1;
  for (std::size_t i = 0; i < series.size(); ++i) {
    const Point point = place_sample(series.at(i), series.newest_time(), series.window(),
                                     range, static_cast<double>(columns),
                                     static_cast<double>(rows), 0.0);
    const int row = static_cast<int>(point.down);
    if (row < top) top = row;
    if (row > bottom) bottom = row;
  }
  return bottom < 0 ? 0 : bottom - top + 1;
}

}  // namespace

RC_TEST("an empty series has nothing in it and says so safely") {
  const Series series(kCapacity, kWindow);
  RC_CHECK(series.empty());
  RC_CHECK_EQ(series.size(), static_cast<std::size_t>(0));

  const Range range = range_of(series);
  RC_CHECK(span(range) > 0.0);   // and it is still a drawable axis
}

RC_TEST("samples arrive and are kept in order, oldest first") {
  Series series(kCapacity, kWindow);
  series.add(1.0, 10.0);
  series.add(2.0, 20.0);
  series.add(3.0, 30.0);

  RC_REQUIRE_EQ(series.size(), static_cast<std::size_t>(3));
  RC_CHECK_NEAR(series.at(0).time, 1.0, 1e-12);
  RC_CHECK_NEAR(series.at(2).value, 30.0, 1e-12);
  RC_CHECK_NEAR(series.oldest_time(), 1.0, 1e-12);
  RC_CHECK_NEAR(series.newest_time(), 3.0, 1e-12);
}

RC_TEST("samples older than the window are dropped") {
  Series series(kCapacity, 5.0);
  for (int i = 0; i <= 20; ++i) series.add(static_cast<double>(i), 1.0);

  // The newest is at t=20, so nothing before t=15 should remain.
  RC_CHECK(series.oldest_time() >= 15.0);
  RC_CHECK_NEAR(series.newest_time(), 20.0, 1e-12);
}

RC_TEST("the window covers the same amount of time at any sample rate") {
  // The check that catches a series dropping by count instead of by age. Two
  // runs of the same signal at different rates must show the same seconds, or
  // no two charts can be compared.
  Series slow(kCapacity, 5.0);
  Series fast(kCapacity, 5.0);

  for (int i = 0; i <= 100; ++i) slow.add(static_cast<double>(i) * 0.5, 1.0);
  for (int i = 0; i <= 1000; ++i) fast.add(static_cast<double>(i) * 0.05, 1.0);

  const double slow_span = slow.newest_time() - slow.oldest_time();
  const double fast_span = fast.newest_time() - fast.oldest_time();

  RC_CHECK_NEAR(slow_span, 5.0, 0.6);
  RC_CHECK_NEAR(fast_span, 5.0, 0.1);
  RC_CHECK_NEAR(slow_span, fast_span, 0.6);
}

RC_TEST("the series never grows past its capacity, however fast the samples arrive") {
  Series series(50, 1e9);   // a window long enough never to drop anything
  for (int i = 0; i < 5000; ++i) series.add(static_cast<double>(i) * 0.001, 1.0);

  RC_CHECK_EQ(series.size(), static_cast<std::size_t>(50));
  RC_CHECK_EQ(series.capacity(), static_cast<std::size_t>(50));
}

RC_TEST("the range covers every sample held") {
  Series series(kCapacity, kWindow);
  series.add(1.0, 5.0);
  series.add(2.0, -3.0);
  series.add(3.0, 9.5);

  const Range range = range_of(series);
  RC_CHECK_NEAR(range.low, -3.0, 1e-12);
  RC_CHECK_NEAR(range.high, 9.5, 1e-12);
}

RC_TEST("padding leaves room above and below") {
  const Range padded_range = padded(Range{0.0, 10.0}, 0.1);
  RC_CHECK_NEAR(padded_range.low, -1.0, 1e-12);
  RC_CHECK_NEAR(padded_range.high, 11.0, 1e-12);
  RC_CHECK_NEAR(span(padded_range), 12.0, 1e-12);
}

RC_TEST("padding a flat signal is still flat, which is why it is not the fix") {
  const Range flat = padded(Range{7.0, 7.0}, 0.5);
  RC_CHECK_NEAR(span(flat), 0.0, 1e-12);
}

RC_TEST("a minimum span widens an axis around its middle") {
  const Range widened = at_least(Range{7.0, 7.0}, 2.0);
  RC_CHECK_NEAR(span(widened), 2.0, 1e-12);
  RC_CHECK_NEAR(widened.low, 6.0, 1e-12);
  RC_CHECK_NEAR(widened.high, 8.0, 1e-12);
}

RC_TEST("a minimum span leaves an axis that is already wide enough alone") {
  const Range unchanged = at_least(Range{0.0, 100.0}, 2.0);
  RC_CHECK_NEAR(unchanged.low, 0.0, 1e-12);
  RC_CHECK_NEAR(unchanged.high, 100.0, 1e-12);
}

RC_TEST("a steady signal autoscaled fills the whole chart, which is the lie") {
  // The measurement this lesson exists for. A battery reading steady to within
  // two microvolts, drawn on an axis fitted to it, occupies every row.
  Series series(kCapacity, kWindow);
  for (int i = 0; i < 200; ++i) {
    const double wobble = static_cast<double>((i * 7919) % 3 - 1) * 1e-6;
    series.add(static_cast<double>(i) * 0.05, 12.0 + wobble);
  }

  const Range fitted = range_of(series);
  RC_REQUIRE(span(fitted) < 1e-5);            // it really is steady
  RC_CHECK(rows_used(series, fitted, 60, 20) > 15);   // and it fills the chart

  std::cout << "\n  autoscaled to the signal, span "
            << std::scientific << std::setprecision(1) << span(fitted) << "\n"
            << draw(series, fitted, 60, 8);

  // With a minimum span the same data is a flat line, which is the truth.
  const Range honest = at_least(fitted, 0.1);
  RC_CHECK(rows_used(series, honest, 60, 20) <= 2);

  std::cout << "\n  with a minimum span of 0.1 volts\n"
            << draw(series, honest, 60, 8) << "\n";
}

RC_TEST("a real change is still visible once a minimum span is in place") {
  // The minimum must not hide anything worth seeing. A tenth of a volt of sag
  // under load is a real event and has to show.
  Series series(kCapacity, kWindow);
  for (int i = 0; i < 100; ++i) series.add(static_cast<double>(i) * 0.05, 12.0);
  for (int i = 100; i < 200; ++i) series.add(static_cast<double>(i) * 0.05, 11.7);

  const Range honest = at_least(padded(range_of(series), 0.1), 0.1);
  RC_CHECK(rows_used(series, honest, 60, 20) > 10);

  std::cout << "  a real sag of three tenths of a volt, same axis rule\n"
            << draw(series, honest, 60, 8) << "\n";
}

RC_TEST("time runs left to right, with the newest sample at the right edge") {
  Series series(kCapacity, 10.0);
  series.add(0.0, 1.0);
  series.add(5.0, 1.0);
  series.add(10.0, 1.0);

  const Range range{0.0, 2.0};
  const Point oldest = place_sample(series.at(0), series.newest_time(), series.window(),
                                    range, 100.0, 20.0, 0.0);
  const Point newest = place_sample(series.at(2), series.newest_time(), series.window(),
                                    range, 100.0, 20.0, 0.0);

  RC_CHECK(oldest.across < newest.across);
  RC_CHECK_NEAR(newest.across, 100.0, 1e-9);   // the right edge
  RC_CHECK_NEAR(oldest.across, 0.0, 1e-9);     // and a full window ago
}

RC_TEST("a higher value is drawn nearer the top") {
  Series series(kCapacity, 10.0);
  series.add(0.0, 0.0);
  series.add(1.0, 10.0);

  const Range range{0.0, 10.0};
  const Point low = place_sample(series.at(0), series.newest_time(), series.window(),
                                 range, 100.0, 20.0, 0.0);
  const Point high = place_sample(series.at(1), series.newest_time(), series.window(),
                                  range, 100.0, 20.0, 0.0);
  RC_CHECK(high.down < low.down);
}

RC_TEST("an axis with no span still produces a finite position") {
  Series series(kCapacity, 10.0);
  series.add(0.0, 5.0);

  const Point point = place_sample(series.at(0), series.newest_time(), series.window(),
                                   Range{5.0, 5.0}, 100.0, 20.0, 0.0);
  RC_CHECK(std::isfinite(point.across));
  RC_CHECK(std::isfinite(point.down));
}

RC_TEST("a gap in the data is drawn as a gap, not as a straight line of samples") {
  // Nothing arrived between five and nine seconds. The chart must leave that
  // part of the time axis empty rather than spreading the samples it does have
  // evenly across the width.
  Series series(kCapacity, 10.0);
  for (int i = 0; i <= 5; ++i) series.add(static_cast<double>(i), 1.0);
  series.add(9.0, 1.0);
  series.add(10.0, 1.0);

  const Range range{0.0, 2.0};
  const Point before_gap = place_sample(series.at(5), series.newest_time(), series.window(),
                                        range, 100.0, 20.0, 0.0);
  const Point after_gap = place_sample(series.at(6), series.newest_time(), series.window(),
                                       range, 100.0, 20.0, 0.0);

  // Four seconds of a ten second window is forty percent of the width.
  RC_CHECK_NEAR(after_gap.across - before_gap.across, 40.0, 1e-6);
}
