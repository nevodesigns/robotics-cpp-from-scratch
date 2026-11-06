#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

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

struct Result {
  double rms = 0.0;
  double worst = 0.0;
};

enum class Method { kGyroOnly, kNaive, kWrapped };

// A robot turning at 0.2 rad/s for two minutes, which is nearly four turns, so that its
// heading crosses the back of the circle several times. The gyro reads 0.02 rad/s too
// high, a bias, with a little noise, at 20 Hz. The compass is read at 5 Hz and is off by
// 0.15 rad of noise. The error is always the short way round: how far the estimate is
// from the truth, not how many turns.
Result run(Method method, double gain) {
  Rng rng{4242};
  const double dt = 0.05;
  double truth = 0.0;
  double estimate = 0.0;
  HeadingFilter filter(gain);
  double sum = 0.0;
  double worst = 0.0;
  int n = 0;

  for (int i = 0; i < 2400; ++i) {
    const double rate = 0.2;
    truth = wrap_pi(truth + rate * dt);
    const double gyro = rate + 0.02 + 0.005 * rng.gaussian();

    if (method == Method::kWrapped) filter.predict(gyro, dt);
    else estimate = wrap_pi(estimate + gyro * dt);

    if (i % 4 == 0) {
      const double compass = wrap_pi(truth + 0.15 * rng.gaussian());
      if (method == Method::kWrapped) {
        filter.correct(compass);
      } else if (method == Method::kNaive) {
        // The plain difference, which is 6.2 where it should be 0.08.
        estimate = estimate + gain * (compass - estimate);
      }
    }

    const double heading = method == Method::kWrapped ? filter.heading() : estimate;
    const double error = std::fabs(wrap_pi(heading - truth));
    worst = std::fmax(worst, error);
    if (i >= 200) {
      sum += error * error;
      ++n;
    }
  }

  Result r;
  r.rms = std::sqrt(sum / static_cast<double>(n));
  r.worst = worst;
  return r;
}

}  // namespace

RC_TEST("an angle is brought into the range, however far out it starts") {
  RC_CHECK_NEAR(wrap_pi(0.0), 0.0, 1e-15);
  RC_CHECK_NEAR(wrap_pi(3.2), 3.2 - kTurn, 1e-12);
  RC_CHECK_NEAR(wrap_pi(-3.2), -3.2 + kTurn, 1e-12);
  RC_CHECK_NEAR(wrap_pi(kTurn), 0.0, 1e-12);
  RC_CHECK_NEAR(wrap_pi(1.0 + 40.0 * kTurn), 1.0, 1e-9);     // forty turns round
  RC_CHECK_NEAR(wrap_pi(1.0 - 1000.0 * kTurn), 1.0, 1e-8);

  // The range is (-pi, pi]: pi is in it and minus pi is not, so that the two ends are
  // one answer and not two.
  RC_CHECK_NEAR(wrap_pi(kPi), kPi, 1e-15);
  RC_CHECK_NEAR(wrap_pi(-kPi), kPi, 1e-15);

  // Everything comes out in range, and in the same direction it went in.
  for (int i = -300; i <= 300; ++i) {
    const double a = static_cast<double>(i) * 0.137;
    const double w = wrap_pi(a);
    RC_CHECK(w > -kPi && w <= kPi);
    RC_CHECK_NEAR(std::sin(w), std::sin(a), 1e-9);
    RC_CHECK_NEAR(std::cos(w), std::cos(a), 1e-9);
  }
}

RC_TEST("the difference between two angles is the short way round") {
  const double plain = 3.1 - (-3.1);

  std::cout << "  3.1 and -3.1 radians, which are on either side of the back of the circle:\n"
            << "    subtracted                " << std::fixed << std::setprecision(3) << plain << "\n"
            << "    the short way round       " << angle_difference(3.1, -3.1) << "\n\n";

  RC_CHECK_NEAR(plain, 6.2, 1e-12);
  RC_CHECK_NEAR(angle_difference(3.1, -3.1), 6.2 - kTurn, 1e-12);   // -0.083
  RC_CHECK_NEAR(angle_difference(-3.1, 3.1), kTurn - 6.2, 1e-12);   // and the other way
  RC_CHECK_NEAR(angle_difference(0.5, 0.2), 0.3, 1e-12);
  RC_CHECK_NEAR(angle_difference(0.2, 0.5), -0.3, 1e-12);

  // Never more than half a turn either way.
  for (int i = -50; i <= 50; ++i) {
    for (int j = -50; j <= 50; ++j) {
      const double d = angle_difference(static_cast<double>(i) * 0.31, static_cast<double>(j) * 0.29);
      RC_CHECK(d > -kPi && d <= kPi);
    }
  }
}

RC_TEST("the average of two angles is the direction between them, and can be undefined") {
  const double near_back[] = {3.1, -3.1};
  bool defined = false;
  const double mean = circular_mean(rc::span<const double>(near_back, 2), rc::span<const double>(), defined);
  const double arithmetic = (3.1 + -3.1) / 2.0;

  std::cout << "  the mean of 3.1 and -3.1:\n"
            << "    added and halved          " << std::fixed << std::setprecision(3) << arithmetic
            << "   (pointing the opposite way from both)\n"
            << "    as directions             " << std::fabs(mean) << "\n\n";

  RC_REQUIRE(defined);
  RC_CHECK_NEAR(std::fabs(mean), kPi, 1e-12);
  RC_CHECK_NEAR(arithmetic, 0.0, 1e-12);

  // A weighted one: three times as much of 0.2 as of 0.6.
  const double a[] = {0.2, 0.6};
  const double w[] = {3.0, 1.0};
  const double weighted = circular_mean(rc::span<const double>(a, 2), rc::span<const double>(w, 2), defined);
  RC_CHECK(defined);
  RC_CHECK(weighted > 0.2 && weighted < 0.4);

  // Opposite directions cancel: there is no average direction, and it says so.
  const double opposite[] = {0.0, kPi};
  circular_mean(rc::span<const double>(opposite, 2), rc::span<const double>(), defined);
  RC_CHECK(!defined);
  circular_mean(rc::span<const double>(), rc::span<const double>(), defined);
  RC_CHECK(!defined);
}

RC_TEST("the filter keeps its heading in range, and a correction goes the short way across the back") {
  HeadingFilter filter(0.5, 3.1);

  // The compass says -3.1, which is 0.083 radians on, across the back. The short way is a
  // small step forward, and the long way, which a subtraction would take, is most of a turn.
  filter.correct(-3.1);
  const double after = filter.heading();
  RC_CHECK(std::fabs(angle_difference(after, 3.1)) < 0.05);
  RC_CHECK(std::fabs(angle_difference(after, 3.1)) > 0.01);

  // Four turns of gyro and the heading is still in the range.
  HeadingFilter spinning(0.1);
  for (int i = 0; i < 5000; ++i) {
    spinning.predict(0.5, 0.05);
    RC_CHECK(spinning.heading() > -kPi && spinning.heading() <= kPi);
  }

  // With no compass it is just the gyro, integrated and wrapped.
  HeadingFilter plain(0.1, 0.0);
  for (int i = 0; i < 100; ++i) plain.predict(1.0, 0.05);
  RC_CHECK_NEAR(plain.heading(), 5.0 - kTurn, 1e-9);
}

RC_TEST("subtracting angles turns the filter the long way round at every crossing") {
  const double gains[] = {0.005, 0.02, 0.1, 0.5};

  std::cout << "  a robot turning at 0.2 rad/s for two minutes, nearly four turns, with a gyro that reads\n"
            << "  0.02 rad/s high and a compass with 0.15 rad of noise. The error is the short way round:\n\n"
            << "    gain    plain difference           short way round\n"
            << "            rms        worst           rms        worst\n";
  Result naive[4];
  Result wrapped[4];
  for (int i = 0; i < 4; ++i) {
    naive[i] = run(Method::kNaive, gains[i]);
    wrapped[i] = run(Method::kWrapped, gains[i]);
    std::cout << "    " << std::fixed << std::setprecision(3) << std::setw(5) << gains[i] << std::setw(11) << naive[i].rms
              << std::setw(12) << naive[i].worst << std::setw(13) << wrapped[i].rms << std::setw(12)
              << wrapped[i].worst << "\n";
  }
  const Result gyro = run(Method::kGyroOnly, 0.0);
  std::cout << "\n    the gyro alone, no compass: rms " << gyro.rms << ", worst " << gyro.worst << "\n\n";

  // The plain difference pulls the estimate the long way round at every crossing, so it
  // is wrong by up to half a turn, which is as wrong as an angle can be, and at the higher
  // gains by all of it. It is no better than the gyro it was meant to correct.
  for (int i = 0; i < 4; ++i) {
    RC_CHECK(naive[i].worst > 1.3);
    RC_CHECK(naive[i].rms > 0.4);
    RC_CHECK(wrapped[i].worst < 1.0);
  }
  RC_CHECK(naive[2].worst > 3.0);
  RC_CHECK(naive[3].worst > 3.0);

  // The gyro alone drifts by 0.02 rad/s for 120 s, 2.4 radians, and the filter with the
  // short way round removes nearly all of it.
  RC_CHECK(gyro.worst > 2.0);
  RC_CHECK(gyro.rms > 1.0);
  RC_CHECK(wrapped[2].rms < 0.06);
  RC_CHECK(wrapped[2].rms * 20.0 < gyro.rms);
}

RC_TEST("the gain is a trade between the gyro's drift and the compass's noise") {
  const double gains[] = {0.005, 0.02, 0.1, 0.5};
  Result r[4];
  for (int i = 0; i < 4; ++i) r[i] = run(Method::kWrapped, gains[i]);

  std::cout << "  the short way round, as the gain rises: " << std::fixed << std::setprecision(3) << r[0].rms << ", "
            << r[1].rms << ", " << r[2].rms << ", " << r[3].rms << " rms.\n"
            << "  Too small and the gyro's bias is let in, too large and the compass's noise is.\n\n";

  // Small gains trust the gyro, and carry its bias: the error falls as the gain rises.
  RC_CHECK(r[0].rms > r[1].rms);
  RC_CHECK(r[1].rms > r[2].rms);

  // And past the best there is the noise: the half gain is worse than the tenth.
  RC_CHECK(r[3].rms > r[2].rms);
}
