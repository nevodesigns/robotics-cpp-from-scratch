#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

constexpr int kSamples = 1000000;
constexpr int kSpikeAt = 123457;
constexpr int kColumns = 800;

// A quiet signal with one spike in it.
std::vector<Sample> spiky() {
  std::vector<Sample> samples;
  samples.reserve(kSamples);
  for (int i = 0; i < kSamples; ++i) {
    Sample s;
    s.time = static_cast<double>(i) * 1e-3;
    s.value = 0.001 * std::sin(static_cast<double>(i) * 0.01);
    if (i == kSpikeAt) s.value = 5.0;
    samples.push_back(s);
  }
  return samples;
}

}  // namespace

RC_TEST("drawing every 1250th of a million samples never sees a spike that is not on one") {
  const std::vector<Sample> samples = spiky();
  const int every = kSamples / kColumns;
  double naive_high = -1e9;
  for (int i = 0; i < kSamples; i += every)
    if (samples[static_cast<std::size_t>(i)].value > naive_high) naive_high = samples[static_cast<std::size_t>(i)].value;

  const double t_end = static_cast<double>(kSamples - 1) * 1e-3;
  const std::vector<Column> columns = decimate(samples, 0.0, t_end, kColumns);
  double high = -1e9;
  for (const Column& c : columns)
    if (c.n > 0 && c.high > high) high = c.high;
  std::cout << "  " << kSamples << " samples, a spike of 5 at sample " << kSpikeAt << " (1250 does not divide it): every " << every << "th sample peaks at "
            << naive_high << ", the columns' highest is " << high << "\n\n";
  RC_CHECK(naive_high < 1.0);
  RC_CHECK_NEAR(high, 5.0, 1e-12);
}

RC_TEST("every sample is in exactly one column, and the output has one column per pixel") {
  const std::vector<Sample> samples = spiky();
  const double t_end = static_cast<double>(kSamples - 1) * 1e-3;
  const std::vector<Column> columns = decimate(samples, 0.0, t_end, kColumns);
  long long counted = 0;
  int empty = 0;
  for (const Column& c : columns) {
    counted += c.n;
    if (c.n == 0) ++empty;
  }
  std::cout << "  " << columns.size() << " columns, " << counted << " samples counted, " << empty << " empty\n\n";
  RC_CHECK_EQ(static_cast<int>(columns.size()), kColumns);
  RC_CHECK_EQ(counted, static_cast<long long>(kSamples));      // the sample at exactly t_end is in the last column and not lost
  RC_CHECK_EQ(empty, 0);
}

RC_TEST("a sample on a boundary goes to the column on its right, and samples outside the range are not counted") {
  std::vector<Sample> samples;
  const double times[] = {-1.0, 0.0, 2.5, 5.0, 7.5, 10.0, 10.5};
  for (double t : times) {
    Sample s;
    s.time = t;
    s.value = t;
    samples.push_back(s);
  }
  const std::vector<Column> c = decimate(samples, 0.0, 10.0, 4);     // bins [0,2.5) [2.5,5) [5,7.5) [7.5,10]
  RC_CHECK_EQ(static_cast<int>(c.size()), 4);
  if (c.size() == 4) {
    RC_CHECK_EQ(c[0].n, 1);       // 0.0
    RC_CHECK_EQ(c[1].n, 1);       // 2.5 is on the boundary and goes right
    RC_CHECK_EQ(c[2].n, 1);       // 5.0
    RC_CHECK_EQ(c[3].n, 2);       // 7.5 and 10.0, which is the end and belongs to the last
    RC_CHECK_NEAR(c[3].low, 7.5, 1e-12);
    RC_CHECK_NEAR(c[3].high, 10.0, 1e-12);
  }

  RC_CHECK_EQ(static_cast<int>(decimate(samples, 0.0, 10.0, 0).size()), 0);
  const std::vector<Column> one = decimate(samples, 0.0, 10.0, 1);
  RC_CHECK_EQ(static_cast<int>(one.size()), 1);
  if (one.size() == 1) RC_CHECK_EQ(one[0].n, 5);
  const std::vector<Column> none = decimate(std::vector<Sample>(), 0.0, 10.0, 3);
  RC_CHECK_EQ(static_cast<int>(none.size()), 3);
  if (none.size() == 3) RC_CHECK_EQ(none[1].n, 0);
  const std::vector<Column> backwards = decimate(samples, 10.0, 0.0, 4);       // not a range: nothing is counted, and no division by zero
  RC_CHECK_EQ(static_cast<int>(backwards.size()), 4);
  if (backwards.size() == 4) RC_CHECK_EQ(backwards[0].n + backwards[1].n + backwards[2].n + backwards[3].n, 0);
}

RC_TEST("each column's low and high are the lowest and highest of the samples in it") {
  // A simple repeatable sequence, not in time order.
  std::vector<Sample> samples;
  unsigned long long state = 12345ULL;
  for (int i = 0; i < 5000; ++i) {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    Sample s;
    s.time = static_cast<double>((state >> 33) % 100000) / 1000.0;           // 0 to 100, in no order
    s.value = static_cast<double>((state >> 20) % 2000) / 100.0 - 10.0;
    samples.push_back(s);
  }
  const int columns = 37;
  const std::vector<Column> got = decimate(samples, 0.0, 100.0, columns);
  RC_CHECK_EQ(static_cast<int>(got.size()), columns);
  int wrong = 0;
  int total = 0;
  for (int c = 0; c < columns && c < static_cast<int>(got.size()); ++c) {
    double low = 1e9, high = -1e9;
    int n = 0;
    for (const Sample& s : samples) {
      const double w = 100.0 / columns;
      int bin = static_cast<int>(s.time / w);
      if (bin >= columns) bin = columns - 1;
      if (bin != c) continue;
      ++n;
      if (s.value < low) low = s.value;
      if (s.value > high) high = s.value;
    }
    total += n;
    if (got[static_cast<std::size_t>(c)].n != n || (n > 0 && (got[static_cast<std::size_t>(c)].low != low || got[static_cast<std::size_t>(c)].high != high))) ++wrong;
  }
  std::cout << "  5000 shuffled samples in " << columns << " columns: " << wrong << " columns differ from a brute force count, " << total << " samples placed\n\n";
  RC_CHECK_EQ(wrong, 0);
  RC_CHECK_EQ(total, 5000);
}

RC_TEST("a million samples are drawn with at most two points a column, and a flat column is one point") {
  const std::vector<Sample> samples = spiky();
  const double t_end = static_cast<double>(kSamples - 1) * 1e-3;
  const std::vector<Column> columns = decimate(samples, 0.0, t_end, kColumns);
  const std::size_t points = points_to_draw(columns);
  std::cout << "  " << kSamples << " samples drawn as " << points << " points, at most " << 2 * kColumns << "\n\n";
  RC_CHECK(points > 0);
  RC_CHECK(points <= static_cast<std::size_t>(2 * kColumns));

  std::vector<Column> flat(3);
  flat[0].n = 4; flat[0].low = flat[0].high = 1.0;     // one point
  flat[1].n = 0;                                        // none
  flat[2].n = 2; flat[2].low = 0.0; flat[2].high = 3.0; // two
  RC_CHECK_EQ(static_cast<int>(points_to_draw(flat)), 3);
}
