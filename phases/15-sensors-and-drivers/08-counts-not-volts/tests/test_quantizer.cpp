#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
  double gaussian() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }
};

// Ten bits over zero to 3.3 volts, which is what a small microcontroller has.
const int kBits = 10;
const double kLow = 0.0;
const double kHigh = 3.3;

// The average of n readings of x with noise of `sigma` LSB added before the converter.
double averaged(const Quantizer& q, double x, double sigma, int n, Rng& rng) {
  double sum = 0.0;
  for (int i = 0; i < n; ++i) sum += q.value(q.code(x + sigma * q.lsb() * rng.gaussian()));
  return sum / static_cast<double>(n);
}

// The rms error, in LSB, of the average of n readings, over many inputs spread across
// the range.
double rms_error(const Quantizer& q, double sigma, int n) {
  Rng inputs{77};
  Rng noise{5};
  const int count = 1000;
  double sum = 0.0;
  for (int i = 0; i < count; ++i) {
    const double x = kLow + (kHigh - kLow) * (0.01 + 0.98 * inputs.uniform());
    const double e = averaged(q, x, sigma, n, noise) - x;
    sum += e * e;
  }
  return std::sqrt(sum / static_cast<double>(count)) / q.lsb();
}

}  // namespace

RC_TEST("the steps, the codes and the middle of a step, with the convention stated") {
  // Three bits over zero to eight volts: eight steps of exactly one volt.
  const Quantizer q(3, 0.0, 8.0);

  RC_CHECK_NEAR(q.lsb(), 1.0, 1e-12);
  RC_CHECK_EQ(q.max_code(), 7L);

  RC_CHECK_EQ(q.code(0.0), 0L);
  RC_CHECK_EQ(q.code(0.999), 0L);
  RC_CHECK_EQ(q.code(1.0), 1L);
  RC_CHECK_EQ(q.code(3.5), 3L);
  RC_CHECK_EQ(q.code(7.999), 7L);

  // A code stands for the middle of its step.
  RC_CHECK_NEAR(q.value(0), 0.5, 1e-12);
  RC_CHECK_NEAR(q.value(3), 3.5, 1e-12);
  RC_CHECK_NEAR(q.value(7), 7.5, 1e-12);

  // Eight steps, not seven: dividing by 2^bits - 1 makes the step 1.143 V, and then the
  // code for 1.0 V is 0 where it should be 1, and every reading is a little off.
  RC_CHECK(std::fabs(q.lsb() - 8.0 / 7.0) > 0.1);
}

RC_TEST("outside the range the converter clips, and says nothing about how far out") {
  const Quantizer q(3, 0.0, 8.0);

  RC_CHECK_EQ(q.code(8.0), 7L);      // the top of the range is the last code, not a new one
  RC_CHECK_EQ(q.code(9.0), 7L);
  RC_CHECK_EQ(q.code(1000.0), 7L);   // a thousand volts and nine volts are the same reading
  RC_CHECK_EQ(q.code(-0.1), 0L);
  RC_CHECK_EQ(q.code(-50.0), 0L);

  RC_CHECK(q.saturated(8.0));
  RC_CHECK(q.saturated(9.0));
  RC_CHECK(q.saturated(0.0));
  RC_CHECK(q.saturated(-1.0));
  RC_CHECK(!q.saturated(0.001));
  RC_CHECK(!q.saturated(7.999));
}

RC_TEST("a single reading is wrong by 0.29 of a step on average and never by more than half") {
  const Quantizer q(kBits, kLow, kHigh);

  // Never more than half a step, because the code stands for the middle of its step.
  double worst = 0.0;
  Rng inputs{3};
  for (int i = 0; i < 5000; ++i) {
    const double x = kLow + (kHigh - kLow) * (0.001 + 0.998 * inputs.uniform());
    worst = std::fmax(worst, std::fabs(q.value(q.code(x)) - x) / q.lsb());
  }

  const double rms = rms_error(q, 0.0, 1);
  std::cout << "  ten bits over 0 to 3.3 V: one step is " << std::fixed << std::setprecision(4) << q.lsb() * 1000.0
            << " mV.\n"
            << "  a single reading is wrong by at most " << std::setprecision(3) << worst << " of a step, and by "
            << rms << " rms,\n  against the " << 1.0 / std::sqrt(12.0) << " that a uniform error of one step predicts.\n\n";

  RC_CHECK(worst <= 0.5 + 1e-9);
  RC_CHECK_NEAR(q.lsb() * 1000.0, 3.2227, 1e-3);
  RC_CHECK_NEAR(rms, 1.0 / std::sqrt(12.0), 0.02);
}

RC_TEST("averaging readings of a steady voltage with no noise gains nothing at all") {
  const Quantizer q(kBits, kLow, kHigh);

  // All the readings are the same code, so their average is that code's value, and the
  // error is exactly what one reading had.
  Rng noise{9};
  for (const double x : {0.4713, 1.2345, 2.7182, 3.0001}) {
    const double one = averaged(q, x, 0.0, 1, noise);
    const double thousand = averaged(q, x, 0.0, 1000, noise);
    RC_CHECK_NEAR(one, thousand, 1e-12);   // the same code a thousand times, up to the rounding of adding them
  }

  const double single = rms_error(q, 0.0, 1);
  const double hundred = rms_error(q, 0.0, 100);
  const double thousand = rms_error(q, 0.0, 1000);
  std::cout << "  no noise on the input, rms error in steps: " << std::fixed << std::setprecision(4) << single
            << " from one reading, " << hundred << " from the average of 100,\n  and " << thousand
            << " from the average of 1000.\n\n";

  RC_CHECK_NEAR(hundred, single, 0.01);
  RC_CHECK_NEAR(thousand, single, 0.01);
}

RC_TEST("noise of about a step makes averaging work, because it moves the input across the boundaries") {
  const Quantizer q(kBits, kLow, kHigh);
  const double sigmas[] = {0.0, 0.3, 1.0, 3.0};
  const int counts[] = {1, 100, 1000};

  std::cout << "  the rms error, in steps, of the average of n readings with noise of sigma steps added\n"
            << "  to the input before the converter:\n\n"
            << "    noise      n = 1     n = 100    n = 1000\n";
  double table[4][3];
  for (int s = 0; s < 4; ++s) {
    std::cout << "    " << std::fixed << std::setprecision(1) << std::setw(4) << sigmas[s] << std::setprecision(4);
    for (int c = 0; c < 3; ++c) {
      table[s][c] = rms_error(q, sigmas[s], counts[c]);
      std::cout << std::setw(11) << table[s][c];
    }
    std::cout << "\n";
  }
  std::cout << "\n  With none, a thousand readings are no better than one. With a step of noise, a hundred give\n"
            << "  a tenth, and a thousand a thirtieth. Past that the noise is the error and goes as one over the\n"
            << "  square root of n.\n\n";

  // No noise: nothing gained.
  RC_CHECK(table[0][2] > 0.27);

  // Noise of 0.3 of a step is already enough to start.
  RC_CHECK(table[1][1] < 0.1);

  // About a step: error falls as one over the square root of the count.
  RC_CHECK(table[2][1] < 0.15);
  RC_CHECK(table[2][2] < 0.05);
  RC_CHECK(table[2][1] / table[2][2] > 2.5);

  // And a great deal of noise is still averaged away, but starts from further out.
  RC_CHECK(table[3][0] > 2.0);
  RC_CHECK(table[3][2] < 0.2);
}

RC_TEST("a change of a third of a step is invisible without noise two times in three, and visible with it") {
  const Quantizer q(kBits, kLow, kHigh);
  const double step = q.lsb() / 3.0;   // the signal moves by a third of a step

  // The fraction of starting voltages at which the average of 1000 readings moves by at
  // least half of what the signal did.
  const auto seen = [&](double sigma) {
    Rng inputs{101};
    Rng noise{13};
    int detected = 0;
    const int trials = 600;
    for (int i = 0; i < trials; ++i) {
      const double x = kLow + (kHigh - kLow) * (0.05 + 0.9 * inputs.uniform());
      const double before = averaged(q, x, sigma, 1000, noise);
      const double after = averaged(q, x + step, sigma, 1000, noise);
      if (std::fabs(after - before) > step / 2.0) ++detected;
    }
    return static_cast<double>(detected) / static_cast<double>(trials);
  };

  const double without = seen(0.0);
  const double with = seen(1.0);

  std::cout << "  the signal moves by a third of a step. The average of 1000 readings notices it:\n"
            << "    with no noise on the input    " << std::fixed << std::setprecision(2) << without * 100.0
            << " percent of the time\n"
            << "    with a step of noise          " << with * 100.0 << " percent of the time\n\n"
            << "  and when it does notice with no noise, it sees a whole step, three times too much.\n\n";

  // It is seen only when the change happens to carry the voltage over a boundary, which is
  // a third of the time.
  RC_CHECK(without > 0.2 && without < 0.45);
  RC_CHECK(with > 0.95);
}
