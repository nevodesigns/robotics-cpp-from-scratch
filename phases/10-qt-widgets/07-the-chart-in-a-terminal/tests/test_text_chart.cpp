#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

std::vector<Sample> make(int n, double (*f)(int)) {
  std::vector<Sample> samples;
  for (int i = 0; i < n; ++i) {
    Sample s;
    s.time = static_cast<double>(i);
    s.value = f(i);
    samples.push_back(s);
  }
  return samples;
}

double steady_with_noise(int i) { return 5.0 + ((i % 2) ? 1e-6 : -1e-6); }
double sine(int i) { return std::sin(static_cast<double>(i) * 2.0 * 3.141592653589793 / 200.0); }
double quiet_with_spike(int i) { return i == 4321 ? 5.0 : 0.001 * std::sin(static_cast<double>(i) * 0.1); }

// Rows that have at least one '*' in the chart part of the line.
int rows_marked(const std::vector<std::string>& lines) {
  int n = 0;
  for (const std::string& line : lines) {
    const std::size_t bar = line.find('|');
    if (bar == std::string::npos) continue;
    if (line.find('*', bar) != std::string::npos) ++n;
  }
  return n;
}

}  // namespace

RC_TEST("noise in the last digit of a steady signal fills the chart unless the range has a minimum span") {
  const std::vector<Sample> samples = make(400, steady_with_noise);
  const std::vector<std::string> raw = render_text(samples, 0.0, 399.0, 40, 13, 0.0);
  const std::vector<std::string> held = render_text(samples, 0.0, 399.0, 40, 13, 1.0);
  std::cout << "  5 plus or minus 1e-6, 13 rows: scaled to the data it marks " << rows_marked(raw) << " rows, with a minimum span of 1 it marks "
            << rows_marked(held) << "\n";
  for (const std::string& line : held) std::cout << "    " << line << "\n";
  std::cout << "\n";
  RC_CHECK_EQ(static_cast<int>(raw.size()), 13);
  RC_CHECK_EQ(static_cast<int>(held.size()), 13);
  RC_CHECK(rows_marked(raw) >= 10);               // the storm: two values 2e-6 apart fill the chart from the top to the bottom, less the padding
  RC_CHECK_EQ(rows_marked(held), 1);              // the flat line
  if (held.size() == 13) {
    const std::size_t bar = held[6].find('|');
    RC_CHECK(bar != std::string::npos);
    if (bar != std::string::npos) RC_CHECK_EQ(static_cast<int>(std::count(held[6].begin() + static_cast<long>(bar), held[6].end(), '*')), 40);
    RC_CHECK(held[6].find("5.0") != std::string::npos);     // and the middle row is labelled with the value it is at
  }
}

RC_TEST("the range has five percent of room, and is never narrower than the minimum span") {
  std::vector<Column> cols(3);
  cols[0].n = 2; cols[0].low = 1.0; cols[0].high = 2.0;
  cols[1].n = 0;
  cols[2].n = 1; cols[2].low = 3.0; cols[2].high = 3.0;
  const Range r = value_range(cols, 0.0);
  RC_CHECK_NEAR(r.low, 0.9, 1e-12);        // 1 to 3 is a span of 2: 0.1 of room below
  RC_CHECK_NEAR(r.high, 3.1, 1e-12);

  const Range wide = value_range(cols, 10.0);   // 10 is wider than the data: centred on it
  RC_CHECK_NEAR(wide.high - wide.low, 10.0, 1e-9);
  RC_CHECK_NEAR((wide.low + wide.high) / 2.0, 2.0, 1e-9);

  const Range none = value_range(std::vector<Column>(4), 0.5);
  RC_CHECK_NEAR(none.low, 0.0, 1e-12);
  RC_CHECK_NEAR(none.high, 1.0, 1e-12);

  const Range flat_range{0.0, 10.0};
  RC_CHECK_EQ(row_for(10.0, flat_range, 11), 0);
  RC_CHECK_EQ(row_for(0.0, flat_range, 11), 10);
  RC_CHECK_EQ(row_for(5.0, flat_range, 11), 5);
  RC_CHECK_EQ(row_for(99.0, flat_range, 11), 0);        // kept inside the chart
  RC_CHECK_EQ(row_for(-99.0, flat_range, 11), 10);
  RC_CHECK_EQ(row_for(3.0, Range{2.0, 2.0}, 9), 4);     // no height: the middle
}

RC_TEST("a sine wave marks every column, from near the top row to near the bottom") {
  const std::vector<Sample> samples = make(400, sine);
  const std::vector<std::string> lines = render_text(samples, 0.0, 399.0, 40, 13, 0.1);
  RC_CHECK_EQ(static_cast<int>(lines.size()), 13);
  int marked_columns = 0;
  int first_row = -1, last_row = -1;
  if (lines.size() == 13) {
    const std::size_t bar = lines[0].find('|') + 1;
    for (std::size_t c = 0; c < 40; ++c) {
      bool any = false;
      for (const std::string& line : lines)
        if (line[bar + c] == '*') any = true;
      if (any) ++marked_columns;
    }
    for (int r = 0; r < 13; ++r) {
      if (lines[static_cast<std::size_t>(r)].find('*') == std::string::npos) continue;
      if (first_row < 0) first_row = r;
      last_row = r;
    }
  }
  std::cout << "  two periods of a sine, 40 columns, 13 rows: " << marked_columns << " columns marked, rows " << first_row << " to " << last_row << "\n\n";
  RC_CHECK_EQ(marked_columns, 40);
  RC_CHECK(first_row >= 0 && first_row <= 1);
  RC_CHECK(last_row >= 11 && last_row <= 12);
}

RC_TEST("a spike in a quiet signal is the highest mark, alone, and it is drawn however many samples there are") {
  const std::vector<Sample> samples = make(10000, quiet_with_spike);
  const std::vector<std::string> lines = render_text(samples, 0.0, 9999.0, 50, 15, 0.0);
  RC_CHECK_EQ(static_cast<int>(lines.size()), 15);
  int stars_in_top = 0;
  int top_row = -1;
  int rows_with_ink = 0;
  if (lines.size() == 15) {
    for (int r = 0; r < 15 && top_row < 0; ++r) {
      const std::size_t bar = lines[static_cast<std::size_t>(r)].find('|');
      const int n = static_cast<int>(std::count(lines[static_cast<std::size_t>(r)].begin() + static_cast<long>(bar), lines[static_cast<std::size_t>(r)].end(), '*'));
      if (n > 0) {
        top_row = r;
        stars_in_top = n;
      }
    }
    rows_with_ink = rows_marked(lines);
  }
  std::cout << "  10000 quiet samples with a spike of 5: the highest mark is in row " << top_row << " and is " << stars_in_top << " mark, " << rows_with_ink
            << " rows with any ink\n\n";
  RC_CHECK(top_row >= 0 && top_row <= 1);                // room for the padding above it, and no more
  RC_CHECK_EQ(stars_in_top, 1);
  RC_CHECK(rows_with_ink >= 2);
}

RC_TEST("every line is the same length, the labels are on tick rows and fall as the rows go down, and bad sizes give no lines") {
  const std::vector<Sample> samples = make(400, sine);
  const std::vector<std::string> lines = render_text(samples, 0.0, 399.0, 30, 13, 0.1);
  RC_CHECK_EQ(static_cast<int>(lines.size()), 13);
  int equal_length = 0;
  int labelled = 0;
  double previous = 1e9;
  int falling = 0;
  for (const std::string& line : lines) {
    if (line.size() == lines[0].size()) ++equal_length;
    const std::size_t bar = line.find('|');
    if (bar == std::string::npos) continue;
    std::string label = line.substr(0, bar);
    while (!label.empty() && label.back() == ' ') label.pop_back();
    while (!label.empty() && label.front() == ' ') label.erase(0, 1);
    if (label.empty()) continue;
    ++labelled;
    const double v = std::atof(label.c_str());
    if (v < previous) ++falling;
    previous = v;
  }
  std::cout << "  " << lines.size() << " lines, " << equal_length << " of the same length, " << labelled << " labelled, " << falling << " of them lower than the one above\n\n";
  RC_CHECK_EQ(equal_length, 13);
  RC_CHECK(labelled >= 2 && labelled <= 8);
  RC_CHECK_EQ(falling, labelled);

  RC_CHECK_EQ(static_cast<int>(render_text(samples, 0.0, 399.0, 0, 13, 0.1).size()), 0);
  RC_CHECK_EQ(static_cast<int>(render_text(samples, 0.0, 399.0, 30, 1, 0.1).size()), 0);
  const std::vector<std::string> empty = render_text(std::vector<Sample>(), 0.0, 10.0, 20, 5, 0.1);
  RC_CHECK_EQ(static_cast<int>(empty.size()), 5);              // an empty chart is still a chart: no marks
  RC_CHECK_EQ(rows_marked(empty), 0);
}
