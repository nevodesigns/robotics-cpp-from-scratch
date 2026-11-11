#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const double kPi = 3.14159265358979323846;

std::vector<Sample> line_stream() {
  std::vector<Sample> s;
  for (int i = 0; i <= 10; ++i) {
    Sample x;
    x.t = i * 0.1;
    x.value = 2.0 * x.t + 1.0;   // exactly a straight line
    s.push_back(x);
  }
  return s;
}

// A sine of amplitude 1 at `hz`, sampled at 10 Hz from 0 to 10 s.
std::vector<Sample> sine_stream(double hz) {
  std::vector<Sample> s;
  for (int i = 0; i <= 100; ++i) {
    Sample x;
    x.t = i * 0.1;
    x.value = std::sin(2.0 * kPi * hz * x.t);
    s.push_back(x);
  }
  return s;
}

}  // namespace

RC_TEST("between two samples a straight line is drawn, and at a sample the sample itself is given") {
  const std::vector<Sample> s = line_stream();
  double v = 0.0;
  RC_REQUIRE(interpolate_at(s, 0.25, 1.0, v));
  RC_CHECK_NEAR(v, 1.5, 1e-12);
  RC_REQUIRE(interpolate_at(s, 0.5, 1.0, v));
  RC_CHECK_NEAR(v, 2.0, 1e-12);
  RC_REQUIRE(interpolate_at(s, 0.0, 1.0, v));
  RC_CHECK_NEAR(v, 1.0, 1e-12);
  RC_REQUIRE(interpolate_at(s, 1.0, 1.0, v));    // exactly the last sample
  RC_CHECK_NEAR(v, 3.0, 1e-12);

  double h = 0.0;
  RC_REQUIRE(hold_at(s, 0.25, h));
  RC_CHECK_NEAR(h, 1.4, 1e-12);                  // the value at 0.2
  RC_REQUIRE(hold_at(s, s[3].t, h));
  RC_CHECK_NEAR(h, 1.6, 1e-12);                  // at a sample: that sample
  RC_REQUIRE(hold_at(s, 99.0, h));
  RC_CHECK_NEAR(h, 3.0, 1e-12);                  // the newest, however old
}

RC_TEST("before the first sample, after the last, and with nothing at all, no value is made up") {
  const std::vector<Sample> s = line_stream();
  double v = 42.0;
  RC_CHECK(!interpolate_at(s, -0.01, 1.0, v));
  RC_CHECK(!interpolate_at(s, 1.01, 1.0, v));
  RC_CHECK_NEAR(v, 42.0, 1e-12);                 // untouched
  const std::vector<Sample> none;
  RC_CHECK(!interpolate_at(none, 0.5, 1.0, v));
  RC_CHECK(!hold_at(none, 0.5, v));
  RC_CHECK(!hold_at(s, -0.01, v));
  RC_CHECK_NEAR(v, 42.0, 1e-12);

  // A readable one first, so the refusals above are about the instant and not about a stream that never worked.
  RC_CHECK(interpolate_at(s, 0.55, 1.0, v));
}

RC_TEST("across a hole in the stream no line is drawn, however many instants fall in it") {
  // 10 Hz, with the samples between 1.0 and 1.6 s lost. Whatever the signal did in there, the stream cannot say.
  std::vector<Sample> s;
  for (int i = 0; i <= 10; ++i) s.push_back(Sample{i * 0.1, 0.0});
  for (int i = 16; i <= 25; ++i) s.push_back(Sample{i * 0.1, 0.0});
  std::vector<double> times;
  for (int i = 0; i <= 250; ++i) times.push_back(i * 0.01);

  const std::vector<Sample> guarded = resample(s, times, 0.15);
  const std::vector<Sample> unguarded = resample(s, times, 1e9);
  long inside_hole = 0;
  for (const Sample& x : guarded)
    if (x.t > 1.0 && x.t < 1.6) ++inside_hole;
  std::cout << "  251 instants asked for, a hole from 1.0 s to 1.6 s: with max_gap 0.15 s " << guarded.size()
            << " answered, with none " << unguarded.size() << "\n  answered inside the hole: " << inside_hole
            << " of the 59 instants there\n\n";
  RC_CHECK(guarded.size() > 0);
  RC_CHECK(inside_hole == 0);
  RC_CHECK(unguarded.size() == times.size());
  RC_CHECK(guarded.size() == times.size() - 59);
}

RC_TEST("a held value lags by half a sample on average, and a line is far closer, by the amounts the maths gives") {
  std::cout << "  a sine of amplitude 1 sampled at 10 Hz, read at 100 instants a second\n"
            << "  signal (Hz)   held, rms error   predicted   line, rms error   predicted\n";
  bool close = true;
  double hold_one = 0.0;
  double line_one = 0.0;
  for (double hz : {0.25, 0.5, 1.0, 2.0}) {
    const std::vector<Sample> s = sine_stream(hz);
    double sum_hold = 0.0;
    double sum_line = 0.0;
    int n = 0;
    for (int i = 100; i <= 899; ++i) {            // from 1 s to 9 s, away from the ends
      const double t = i * 0.01;
      const double truth = std::sin(2.0 * kPi * hz * t);
      double held = 0.0;
      double line = 0.0;
      RC_REQUIRE(hold_at(s, t, held));
      RC_REQUIRE(interpolate_at(s, t, 1.0, line));
      sum_hold += (held - truth) * (held - truth);
      sum_line += (line - truth) * (line - truth);
      ++n;
    }
    const double w = 2.0 * kPi * hz;
    const double hold_rms = std::sqrt(sum_hold / n);
    const double line_rms = std::sqrt(sum_line / n);
    const double hold_pred = w / std::sqrt(2.0) * 0.1 / std::sqrt(3.0);
    const double line_pred = w * w / std::sqrt(2.0) * 0.01 / (2.0 * std::sqrt(30.0));
    std::cout << "  " << std::setw(11) << std::fixed << std::setprecision(2) << hz << "   " << std::setw(15)
              << std::setprecision(4) << hold_rms << "   " << std::setw(9) << hold_pred << "   " << std::setw(15)
              << line_rms << "   " << std::setw(9) << line_pred << "\n";
    if (hz <= 1.0 && std::fabs(hold_rms - hold_pred) > 0.2 * hold_pred) close = false;
    if (hz <= 1.0 && std::fabs(line_rms - line_pred) > 0.25 * line_pred) close = false;
    if (hz == 1.0) {
      hold_one = hold_rms;
      line_one = line_rms;
    }
  }
  std::cout << "\n";
  RC_CHECK(close);
  RC_CHECK(hold_one > 0.0);
  RC_CHECK(line_one < 0.2 * hold_one);
}

RC_TEST("two sensors on the same quantity disagree by the hold's lag, and agree once the slow one is put on the fast one's instants") {
  // A fast stream at 100 Hz and a slow one at 10 Hz, both reading the same 1 Hz sine exactly.
  const std::vector<Sample> slow = sine_stream(1.0);
  std::vector<double> instants;
  std::vector<Sample> fast;
  for (int i = 100; i <= 899; ++i) {
    const double t = i * 0.01;
    instants.push_back(t);
    fast.push_back(Sample{t, std::sin(2.0 * kPi * t)});
  }
  const std::vector<Sample> aligned = resample(slow, instants, 1.0);
  RC_REQUIRE(aligned.size() == fast.size());
  double sum_hold = 0.0;
  double sum_line = 0.0;
  double worst_hold = 0.0;
  for (std::size_t i = 0; i < fast.size(); ++i) {
    double held = 0.0;
    RC_REQUIRE(hold_at(slow, fast[i].t, held));
    const double dh = held - fast[i].value;
    const double dl = aligned[i].value - fast[i].value;
    sum_hold += dh * dh;
    sum_line += dl * dl;
    if (std::fabs(dh) > worst_hold) worst_hold = std::fabs(dh);
  }
  const double rms_hold = std::sqrt(sum_hold / fast.size());
  const double rms_line = std::sqrt(sum_line / fast.size());
  std::cout << "  the two streams read the same signal exactly. Disagreement, rms: slow stream held " << std::fixed
            << std::setprecision(4) << rms_hold << ", slow stream interpolated " << rms_line << "; worst held " << worst_hold << "\n\n";
  RC_CHECK(rms_hold > 0.0);
  RC_CHECK(rms_line < 0.2 * rms_hold);
  RC_CHECK(aligned.front().t == fast.front().t);
}
