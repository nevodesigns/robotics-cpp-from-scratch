#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

ChartView standard() {
  ChartView v;
  v.t0 = 0.0;
  v.t1 = 100.0;
  v.range = Range{0.0, 1.0};
  v.across = 1000.0;
  v.down = 100.0;
  v.margin = 0.0;
  return v;
}

Sample at(double time, double value) {
  Sample s;
  s.time = time;
  s.value = value;
  return s;
}

}  // namespace

RC_TEST("a point goes to a pixel and back to itself, and a higher value is a smaller y") {
  ChartView v = standard();
  v.margin = 10.0;
  v.t0 = 5.0;
  v.t1 = 15.0;
  v.range = Range{-2.0, 6.0};
  int off = 0;
  const double times[] = {5.0, 7.25, 10.0, 14.9, 15.0};
  const double values[] = {-2.0, -0.5, 2.0, 5.5, 6.0};
  for (double t : times) {
    for (double value : values) {
      const PixelPoint p = to_pixel(v, t, value);
      const DataPoint d = from_pixel(v, p.x, p.y);
      if (std::fabs(d.time - t) > 1e-9 || std::fabs(d.value - value) > 1e-9) ++off;
    }
  }
  const PixelPoint low = to_pixel(v, 10.0, -2.0);
  const PixelPoint high = to_pixel(v, 10.0, 6.0);
  const PixelPoint start = to_pixel(v, 5.0, 2.0);
  const PixelPoint finish = to_pixel(v, 15.0, 2.0);
  std::cout << "  25 points out and back: " << off << " off by more than 1e-9; the lowest value is at y = " << low.y << " and the highest at y = " << high.y
            << "; time 5 is at x = " << start.x << " and time 15 at x = " << finish.x << "\n\n";
  RC_CHECK_EQ(off, 0);
  RC_CHECK_NEAR(low.y, 90.0, 1e-9);       // the bottom of the usable area: 100 - margin
  RC_CHECK_NEAR(high.y, 10.0, 1e-9);                          // the top, margin from the edge
  RC_CHECK(high.y < low.y);
  RC_CHECK_NEAR(start.x, 10.0, 1e-9);
  RC_CHECK_NEAR(finish.x, 990.0, 1e-9);
}

RC_TEST("the sample nearest a click in pixels is not the nearest in seconds and volts") {
  const ChartView v = standard();              // 10 pixels a second, 100 pixels a volt
  std::vector<Sample> samples;
  samples.push_back(at(50.0, 0.5));          // A
  samples.push_back(at(50.9, 0.9));          // B
  // The click is at the time of A and the value of B.
  const PixelPoint click = to_pixel(v, 50.0, 0.9);
  const int by_pixels = pick(samples, v, click.x, click.y, 100.0);

  const double raw_a = std::hypot(50.0 - 50.0, 0.5 - 0.9);
  const double raw_b = std::hypot(50.9 - 50.0, 0.9 - 0.9);
  const PixelPoint pa = to_pixel(v, 50.0, 0.5);
  const PixelPoint pb = to_pixel(v, 50.9, 0.9);
  const double px_a = std::hypot(pa.x - click.x, pa.y - click.y);
  const double px_b = std::hypot(pb.x - click.x, pb.y - click.y);
  std::cout << "  click at the time of A and the value of B: in raw units A is " << raw_a << " away and B " << raw_b << ", so A is nearer; in pixels A is " << px_a
            << " away and B " << px_b << ", so B is\n\n";
  RC_CHECK(raw_a < raw_b);                    // the wrong answer, from adding seconds to volts
  RC_CHECK(px_b < px_a);
  RC_CHECK_EQ(by_pixels, 1);
}

RC_TEST("a click beyond the tolerance picks nothing, and one exactly at it picks the sample") {
  const ChartView v = standard();
  std::vector<Sample> samples;
  samples.push_back(at(20.0, 0.5));
  const PixelPoint p = to_pixel(v, 20.0, 0.5);
  RC_CHECK_EQ(pick(samples, v, p.x, p.y, 8.0), 0);
  RC_CHECK_EQ(pick(samples, v, p.x + 3.0, p.y + 4.0, 8.0), 0);        // 5 pixels
  RC_CHECK_EQ(pick(samples, v, p.x + 8.0, p.y, 8.0), 0);              // exactly 8: inclusive
  RC_CHECK_EQ(pick(samples, v, p.x + 8.1, p.y, 8.0), -1);
  RC_CHECK_EQ(pick(samples, v, p.x + 6.0, p.y + 6.0, 8.0), -1);       // 8.49 diagonally: beyond it
  RC_CHECK_EQ(pick(std::vector<Sample>(), v, 5.0, 5.0, 8.0), -1);
}

RC_TEST("a click on any of a thousand samples picks that sample, and a tie goes to the earlier one") {
  const ChartView v = standard();
  std::vector<Sample> samples;
  for (int i = 0; i < 1000; ++i) samples.push_back(at(static_cast<double>(i) * 0.1, 0.5 + 0.4 * std::sin(static_cast<double>(i) * 0.05)));
  int wrong = 0;
  int clicks = 0;
  for (int k = 0; k < 1000; k += 7) {
    ++clicks;
    const PixelPoint p = to_pixel(v, samples[static_cast<std::size_t>(k)].time, samples[static_cast<std::size_t>(k)].value);
    if (pick(samples, v, p.x, p.y, 0.4) != k) ++wrong;     // the samples are a pixel apart in x, so a tolerance of half of that is unambiguous
  }
  std::cout << "  " << clicks << " clicks, each on a sample: " << wrong << " picked a different one\n\n";
  RC_CHECK_EQ(clicks, 143);
  RC_CHECK_EQ(wrong, 0);

  std::vector<Sample> twins;
  twins.push_back(at(10.0, 0.5));
  twins.push_back(at(10.0, 0.5));
  const PixelPoint t = to_pixel(v, 10.0, 0.5);
  RC_CHECK_EQ(pick(twins, v, t.x, t.y, 5.0), 0);
}

RC_TEST("samples off the chart cannot be picked, and a view with no size does not divide by zero") {
  const ChartView v = standard();
  std::vector<Sample> samples;
  samples.push_back(at(-5.0, 0.5));
  samples.push_back(at(105.0, 0.5));
  // Their positions would be at x = -50 and x = 1050: a click there must not select them.
  RC_CHECK_EQ(pick(samples, v, -50.0, 50.0, 10.0), -1);
  RC_CHECK_EQ(pick(samples, v, 1050.0, 50.0, 10.0), -1);

  ChartView tiny = standard();
  tiny.across = 10.0;
  tiny.margin = 5.0;                         // a usable width of zero
  const DataPoint d = from_pixel(tiny, 5.0, 5.0);
  RC_CHECK_NEAR(d.time, 0.0, 1e-12);
  RC_CHECK(std::isfinite(d.value));

  ChartView flat = standard();
  flat.range = Range{2.0, 2.0};
  const PixelPoint mid = to_pixel(flat, 50.0, 2.0);
  RC_CHECK_NEAR(mid.y, 50.0, 1e-9);          // no height: the middle
}
