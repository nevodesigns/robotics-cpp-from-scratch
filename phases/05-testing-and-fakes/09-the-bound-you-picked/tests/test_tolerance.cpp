#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const int kReadings = 100;               // readings in the average
const int kSeeds = 10000;                // how many times a test is run to see how often it fails

// A test that checks the average is within `bound` of the truth.
bool within(unsigned seed, int n, double bias, double bound) {
  return std::fabs(noisy_average(seed, n, 5.0, bias) - 5.0) <= bound;
}

}  // namespace

RC_TEST("the mean and the spread of some numbers") {
  const std::vector<double> values = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
  const Spread s = mean_stddev(values);
  RC_CHECK_NEAR(s.mean, 5.0, 1e-12);
  RC_CHECK_NEAR(s.stddev, std::sqrt(32.0 / 7.0), 1e-12);        // the sample standard deviation: sum of squares 32, over n - 1

  const Spread empty = mean_stddev(std::vector<double>());
  RC_CHECK_NEAR(empty.mean, 0.0, 1e-12);
  RC_CHECK_NEAR(empty.stddev, 0.0, 1e-12);
  const Spread one = mean_stddev(std::vector<double>(1, 3.0));
  RC_CHECK_NEAR(one.mean, 3.0, 1e-12);
  RC_CHECK_NEAR(one.stddev, 0.0, 1e-12);

  RC_CHECK_NEAR(bound_for(0.1, 6.0), 0.6, 1e-12);
}

RC_TEST("the number of readings a test needs to see a bug of a given size") {
  RC_CHECK_EQ(samples_needed(1.0, 0.3, 6.0), 1600);
  RC_CHECK_EQ(samples_needed(1.0, 1.0, 6.0), 144);
  RC_CHECK_EQ(samples_needed(1.0, 0.1, 6.0), 14400);
  RC_CHECK_EQ(samples_needed(1.0, 0.0, 6.0), 0);
  RC_CHECK_EQ(samples_needed(1.0, -1.0, 6.0), 0);
  RC_CHECK_EQ(samples_needed(2.0, 1.0, 3.0), 144);                 // twice the noise needs four times the readings
  RC_CHECK_EQ(samples_needed(1.0, 0.7, 3.0), 74);                  // rounded up: (6 / 0.7)^2 = 73.47
}

RC_TEST("the average of a hundred readings varies by about a tenth, so that is the spread a bound has to fit") {
  std::vector<double> averages;
  for (int seed = 1; seed <= kSeeds; ++seed) averages.push_back(noisy_average(static_cast<unsigned>(seed), kReadings, 5.0, 0.0));
  const Spread s = mean_stddev(averages);
  std::cout << "  the average of " << kReadings << " readings over " << kSeeds << " seeds: mean " << std::fixed << std::setprecision(4)
            << s.mean << ", spread " << s.stddev << " (the readings themselves have a spread of 1, so 1 / 10 is expected)\n\n";
  RC_CHECK_NEAR(s.mean, 5.0, 0.005);
  RC_CHECK_NEAR(s.stddev, 0.1, 0.005);
}

RC_TEST("a bound of one spread, which looked fine on the first run, fails about a third of the time") {
  const bool first_run = within(1, kReadings, 0.0, 0.1);
  const int failures = count_failures([](unsigned seed) { return within(seed, kReadings, 0.0, 0.1); }, 1, kSeeds);
  std::cout << "  the first run (seed 1) " << (first_run ? "passes" : "fails") << " this bound\n";
  std::cout << "  bound 0.1 (one spread): failed " << failures << " of " << kSeeds << " runs, " << std::fixed << std::setprecision(1)
            << 100.0 * failures / kSeeds << " percent\n";
  RC_CHECK(first_run);                      // the run that was used to pick the bound is a good one
  RC_CHECK(failures > kSeeds / 4);
  RC_CHECK(failures < kSeeds * 2 / 5);
}

RC_TEST("three spreads fails about once in four hundred, and six spreads did not fail once in ten thousand") {
  const int three = count_failures([](unsigned seed) { return within(seed, kReadings, 0.0, bound_for(0.1, 3.0)); }, 1, kSeeds);
  const int six = count_failures([](unsigned seed) { return within(seed, kReadings, 0.0, bound_for(0.1, 6.0)); }, 1, kSeeds);
  std::cout << "  bound 0.3 (three spreads): failed " << three << " of " << kSeeds << ", bound 0.6 (six spreads): failed " << six
            << " of " << kSeeds << "\n";
  RC_CHECK(three > 8);
  RC_CHECK(three < 60);
  RC_CHECK_EQ(six, 0);
}

RC_TEST("a bound wide enough not to cry wolf still catches a bug, if it is big enough for the readings") {
  const int runs = 2000;
  std::cout << "  six spreads (bound 0.6) with " << kReadings << " readings: how often a bias is seen over " << runs << " seeds\n"
            << "    bias   seen\n";
  const double biases[] = {0.3, 0.5, 0.7, 0.8, 1.0};
  int seen_at_one = 0;
  int seen_at_point_three = 0;
  for (double bias : biases) {
    const int seen = count_failures([bias](unsigned seed) { return within(seed, kReadings, bias, 0.6); }, 100000, runs);
    std::cout << "    " << std::fixed << std::setprecision(1) << bias << "   " << std::setw(5) << std::setprecision(1)
              << 100.0 * seen / runs << " percent\n";
    if (bias == 1.0) seen_at_one = seen;
    if (bias == 0.3) seen_at_point_three = seen;
  }
  RC_CHECK_EQ(seen_at_one, runs);              // a bias of 10 spreads is always seen
  RC_CHECK(seen_at_point_three < runs / 20);   // a bias of 3 spreads is nearly never seen with this bound

  // To see a bias of 0.3 with a bound of six spreads, enough readings are needed.
  const int n = samples_needed(1.0, 0.3, 6.0);
  const double bound = bound_for(1.0 / std::sqrt(static_cast<double>(n)), 6.0);
  const int caught = count_failures([n, bound](unsigned seed) { return within(seed, n, 0.3, bound); }, 200000, runs);
  const int false_alarms = count_failures([n, bound](unsigned seed) { return within(seed, n, 0.0, bound); }, 300000, kSeeds);
  std::cout << "  with " << n << " readings the bound is " << std::setprecision(3) << bound << ": a bias of 0.3 was seen in " << caught
            << " of " << runs << " runs, and there were " << false_alarms << " false alarms in " << kSeeds << "\n\n";
  RC_CHECK_EQ(caught, runs);
  RC_CHECK_EQ(false_alarms, 0);
}
