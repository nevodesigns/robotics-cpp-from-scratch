#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

#include "solution.hpp"

namespace {

struct Scores {
  double odometry = 0.0;
  double measurement = 0.0;
  double fused = 0.0;
};

// A robot driving in a straight line, with an odometer that drifts and a sensor
// that is noisy but does not drift. The filter sees exactly what the robot
// sees: the reported motion, never the true motion.
Scores run(double drift, double sensor, double believed_drift, double believed_sensor,
           unsigned seed) {
  std::mt19937 rng(seed);
  std::normal_distribution<double> wander(0.0, drift);
  std::normal_distribution<double> noise(0.0, sensor);

  double truth = 0.0;
  double dead_reckoned = 0.0;
  Filter1D filter(Estimate{0.0, 1e-6}, believed_drift * believed_drift);

  double sum_odometry = 0.0;
  double sum_measurement = 0.0;
  double sum_fused = 0.0;

  const int steps = 2000;
  const int measure_every = 10;

  for (int i = 0; i < steps; ++i) {
    const double move = 0.05;
    truth += move;

    // One reading of the odometer, used by both. The filter has no access to
    // the true motion, which is the whole point.
    const double reported = move + wander(rng);
    dead_reckoned += reported;
    filter.predict(reported);

    if (i % measure_every == 0) {
      const double seen = truth + noise(rng);
      filter.correct(seen, believed_sensor * believed_sensor);
      sum_measurement += (seen - truth) * (seen - truth);
    }

    sum_odometry += (dead_reckoned - truth) * (dead_reckoned - truth);
    sum_fused += (filter.estimate().value - truth) * (filter.estimate().value - truth);
  }

  return Scores{std::sqrt(sum_odometry / steps),
                std::sqrt(sum_measurement / (steps / measure_every)),
                std::sqrt(sum_fused / steps)};
}

Scores average(double drift, double sensor, double believed_drift, double believed_sensor) {
  Scores total;
  const int trials = 12;
  for (int k = 0; k < trials; ++k) {
    const Scores one = run(drift, sensor, believed_drift, believed_sensor,
                           11 + static_cast<unsigned>(k));
    total.odometry += one.odometry;
    total.measurement += one.measurement;
    total.fused += one.fused;
  }
  return Scores{total.odometry / trials, total.measurement / trials, total.fused / trials};
}

constexpr double kDrift = 0.002;
constexpr double kSensor = 0.10;

}  // namespace

RC_TEST("combining two estimates lands between them") {
  const Estimate combined = fuse(Estimate{0.0, 1.0}, Estimate{10.0, 1.0});
  RC_CHECK_NEAR(combined.value, 5.0, 1e-12);
}

RC_TEST("the answer leans toward whichever is more certain") {
  // Nine times as sure of the second, so the result sits nine tenths of the way
  // across rather than halfway.
  const Estimate leaning = fuse(Estimate{0.0, 9.0}, Estimate{10.0, 1.0});
  RC_CHECK_NEAR(leaning.value, 9.0, 1e-12);

  const Estimate other_way = fuse(Estimate{0.0, 1.0}, Estimate{10.0, 9.0});
  RC_CHECK_NEAR(other_way.value, 1.0, 1e-12);
}

RC_TEST("the combination is more certain than either of its inputs") {
  // Always, and this is the part worth sitting with. Even a poor second opinion
  // leaves you better off than the good one alone.
  const double variances[] = {0.01, 0.25, 1.0, 4.0, 100.0};
  for (const double a : variances) {
    for (const double b : variances) {
      const Estimate combined = fuse(Estimate{0.0, a}, Estimate{1.0, b});
      RC_REQUIRE(combined.variance <= a + 1e-12);
      RC_REQUIRE(combined.variance <= b + 1e-12);
      RC_REQUIRE(combined.variance > 0.0);
    }
  }
}

RC_TEST("combining something with a hopeless guess changes almost nothing") {
  const Estimate good{4.0, 0.01};
  const Estimate hopeless{1000.0, 1e6};
  const Estimate combined = fuse(good, hopeless);

  RC_CHECK_NEAR(combined.value, 4.0, 0.02);
  RC_CHECK(combined.variance < good.variance);
}

RC_TEST("two certainties do not divide by zero") {
  const Estimate combined = fuse(Estimate{3.0, 0.0}, Estimate{7.0, 0.0});
  RC_CHECK(std::isfinite(combined.value));
  RC_CHECK(std::isfinite(combined.variance));
  RC_CHECK_NEAR(combined.value, 3.0, 1e-12);
}

RC_TEST("the gain is the prior's share of the total uncertainty") {
  RC_CHECK_NEAR(gain_toward(Estimate{0.0, 1.0}, Estimate{0.0, 1.0}), 0.5, 1e-12);
  RC_CHECK_NEAR(gain_toward(Estimate{0.0, 3.0}, Estimate{0.0, 1.0}), 0.75, 1e-12);
  RC_CHECK_NEAR(gain_toward(Estimate{0.0, 0.0}, Estimate{0.0, 1.0}), 0.0, 1e-12);
}

RC_TEST("predicting adds uncertainty, and correcting takes it away") {
  // The check that catches a filter which never grows its variance. Such a
  // filter becomes more confident every step it takes on no evidence at all,
  // and eventually stops listening to anything.
  Filter1D filter(Estimate{0.0, 0.01}, 0.001);

  const double before = filter.estimate().variance;
  filter.predict(1.0);
  RC_CHECK(filter.estimate().variance > before);

  const double after_moving = filter.estimate().variance;
  filter.correct(1.0, 0.01);
  RC_CHECK(filter.estimate().variance < after_moving);
}

RC_TEST("a filter that never measures becomes less sure, without limit") {
  Filter1D filter(Estimate{0.0, 0.01}, 0.001);
  for (int i = 0; i < 1000; ++i) filter.predict(0.05);

  RC_CHECK(filter.estimate().variance > 0.9);
  RC_CHECK_NEAR(filter.estimate().value, 50.0, 1e-9);
}

RC_TEST("standard deviation and variance are not the same number") {
  // Five centimetres of accuracy is a variance of 0.0025, not 0.05, and using
  // one where the other belongs is wrong by a factor of twenty.
  const Estimate e{0.0, 0.05 * 0.05};
  RC_CHECK_NEAR(deviation(e), 0.05, 1e-12);
  RC_CHECK_NEAR(e.variance, 0.0025, 1e-12);
}

RC_TEST("fusing beats both of the things it fuses") {
  // The measurement this lesson exists for.
  const Scores s = average(kDrift, kSensor, kDrift, kSensor);

  std::cout << "\n  root mean square error over 2000 steps\n\n"
            << "    dead reckoning alone   " << std::fixed << std::setprecision(4)
            << s.odometry << " m\n"
            << "    the sensor alone       " << s.measurement << " m\n"
            << "    the two combined       " << s.fused << " m\n";

  RC_CHECK(s.fused < s.odometry);
  RC_CHECK(s.fused < s.measurement);
  RC_CHECK(s.fused < s.odometry / 2.0);
}

RC_TEST("believing the wrong uncertainties costs about as much as having worse ones") {
  // Being wrong in either direction roughly doubles the error, and the filter
  // is still better than either input, which is the honest summary: it is
  // forgiving, and you lose most of what it was worth.
  const Scores honest = average(kDrift, kSensor, kDrift, kSensor);
  const Scores too_proud = average(kDrift, kSensor, kDrift / 10.0, kSensor);
  const Scores too_humble = average(kDrift, kSensor, kDrift * 10.0, kSensor);

  std::cout << "\n    believing the truth                          "
            << std::fixed << std::setprecision(4) << honest.fused << " m\n"
            << "    believing its odometry ten times better      " << too_proud.fused << " m\n"
            << "    believing its odometry ten times worse       " << too_humble.fused << " m\n";

  RC_CHECK(too_proud.fused > honest.fused * 1.5);
  RC_CHECK(too_humble.fused > honest.fused * 1.5);
  RC_CHECK(too_proud.fused < honest.fused * 4.0);
}

RC_TEST("only the ratio of the two beliefs matters") {
  // Which is why two mistakes that sound completely different give the same
  // answer: believing the odometry ten times worse and believing the sensor ten
  // times better are the same statement about which to trust.
  const Scores worse_odometry = average(kDrift, kSensor, kDrift * 10.0, kSensor);
  const Scores better_sensor = average(kDrift, kSensor, kDrift, kSensor / 10.0);

  std::cout << "    believing its sensor ten times better        "
            << std::fixed << std::setprecision(4) << better_sensor.fused << " m\n\n";

  RC_CHECK_NEAR(worse_odometry.fused, better_sensor.fused, 0.005);
}
