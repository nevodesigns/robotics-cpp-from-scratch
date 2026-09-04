#include <rc/test/rc_test.hpp>

#include <rc/core/clock.hpp>
#include <rc/nav/filter.hpp>

#include <cmath>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

using rc::core::Nanoseconds;
using rc::nav::Filter1D;

constexpr double kDt = 0.002;             // odometry at 500 Hz
constexpr int kFixEvery = 50;             // an absolute fix at 10 Hz
constexpr double kFixDeviation = 0.10;    // and it is good to 10 cm
constexpr double kOdomDeviation = 0.002;  // per step
constexpr int kSteps = 20000;             // forty seconds
constexpr int kWarmup = 2000;

class Noise {
 public:
  explicit Noise(std::uint64_t seed) : state_(seed * 6364136223846793005ULL + 1ULL) {}
  double gaussian() {
    double total = 0.0;
    for (int i = 0; i < 12; ++i) {
      state_ = state_ * 6364136223846793005ULL + 1442695040888963407ULL;
      total += static_cast<double>((state_ >> 11) & 0x1FFFFFFFFFFFFFULL) /
               static_cast<double>(0x20000000000000ULL);
    }
    return total - 6.0;
  }

 private:
  std::uint64_t state_;
};

// How the robot drives. Constant speed hides things that stop and go reveals,
// which is one of the findings of this lesson rather than a detail of the rig.
enum class Motion { steady, stop_and_go };

double speed_at(Motion motion, int step) {
  if (motion == Motion::steady) return 1.0;
  return (step / 500) % 2 == 0 ? 2.0 : 0.0;
}

// What the fix is fused as.
enum class Method { none, naive, projected, rewound };

struct Outcome {
  double rms = 0.0;         // against the truth
  double claimed = 0.0;     // what the filter said about itself
  double monitor_rms = 0.0; // what the consistency check saw
};

Outcome run(Method method, int late_ms, Motion motion) {
  const int late = late_ms / 2;
  Noise odometry_noise(3), fix_noise(9);

  Filter1D filter(Estimate{0.0, 0.01}, kOdomDeviation * kOdomDeviation);
  ConsistencyMonitor monitor;
  std::deque<double> truth_history, motion_history;
  std::deque<Estimate> filter_history;

  double truth = 0.0;
  double dead_reckoning = 0.0;
  double squared = 0.0, variance_sum = 0.0;
  int counted = 0;

  for (int i = 0; i < kSteps; ++i) {
    const double speed = speed_at(motion, i);
    truth += speed * kDt;

    const double step = speed * kDt + kOdomDeviation * odometry_noise.gaussian();
    dead_reckoning += step;
    filter.predict(step);

    truth_history.push_back(truth);
    motion_history.push_back(step);
    filter_history.push_back(filter.estimate());
    if (static_cast<int>(truth_history.size()) > late + 2) {
      truth_history.pop_front();
      motion_history.pop_front();
      filter_history.pop_front();
    }

    if (method != Method::none && i % kFixEvery == 0 && i >= late) {
      // The fix describes where the robot was when the picture was taken.
      Delayed fix;
      fix.value = truth_history[truth_history.size() - 1 - static_cast<std::size_t>(late)] +
                  kFixDeviation * fix_noise.gaussian();
      fix.variance = kFixDeviation * kFixDeviation;
      fix.sampled_at = static_cast<Nanoseconds>(i - late) * 2000000;

      Estimate used{fix.value, fix.variance};
      if (method == Method::projected) {
        double since = 0.0;
        for (std::size_t k = motion_history.size() - static_cast<std::size_t>(late);
             k < motion_history.size(); ++k)
          since += motion_history[k];
        used = project_forward(fix, since, late * kOdomDeviation * kOdomDeviation);
      }

      if (i >= kWarmup) monitor.add(innovation_of(filter.estimate(), used));

      if (method == Method::rewound) {
        // The textbook answer, done the way it is usually first written: put
        // the filter back to the moment the measurement describes, correct it
        // there, and replay the motion since. It replays only the motion.
        Filter1D back(filter_history[filter_history.size() - 1 - static_cast<std::size_t>(late)],
                      kOdomDeviation * kOdomDeviation);
        back.correct(fix.value, fix.variance);
        for (std::size_t k = motion_history.size() - static_cast<std::size_t>(late);
             k < motion_history.size(); ++k)
          back.predict(motion_history[k]);
        filter = back;
      } else {
        filter.correct(used.value, used.variance);
      }
    }

    if (i >= kWarmup) {
      const double estimate = method == Method::none ? dead_reckoning : filter.estimate().value;
      squared += (estimate - truth) * (estimate - truth);
      variance_sum += filter.estimate().variance;
      ++counted;
    }
  }

  Outcome outcome;
  outcome.rms = std::sqrt(squared / counted);
  outcome.claimed = std::sqrt(variance_sum / counted);
  outcome.monitor_rms = monitor.rms();
  return outcome;
}

}  // namespace

RC_TEST("a measurement moved to the present, and what the move costs it") {
  Delayed fix;
  fix.value = 4.0;
  fix.variance = 0.01;
  fix.sampled_at = 0;

  const Estimate moved = project_forward(fix, 0.08, 0.0004);
  RC_CHECK_NEAR(moved.value, 4.08, 1e-12);
  RC_CHECK_NEAR(moved.variance, 0.0104, 1e-12);

  // Moved nowhere, it is itself.
  const Estimate still = project_forward(fix, 0.0, 0.0);
  RC_CHECK_NEAR(still.value, 4.0, 1e-12);
  RC_CHECK_NEAR(still.variance, 0.01, 1e-12);

  // And the variance only ever grows, because the motion you added was
  // measured rather than known.
  RC_CHECK(moved.variance > fix.variance);
}

RC_TEST("an innovation is a residual divided by what was expected of it") {
  const Estimate prior{5.0, 0.0016};       // the filter, sure to 4 cm
  const Estimate measurement{5.1, 0.01};   // a fix, good to 10 cm

  const Innovation innovation = innovation_of(prior, measurement);
  RC_CHECK_NEAR(innovation.residual, 0.1, 1e-12);
  RC_CHECK_NEAR(innovation.expected_deviation, std::sqrt(0.0116), 1e-12);
  RC_CHECK_NEAR(innovation.normalised(), 0.1 / std::sqrt(0.0116), 1e-12);

  // Ten centimetres from a laser and ten centimetres from a first GPS fix are
  // not the same event, and the normalised form is what says so.
  const Innovation from_gps = innovation_of(Estimate{5.0, 25.0}, Estimate{5.1, 25.0});
  RC_CHECK(std::fabs(from_gps.normalised()) < 0.05);
  RC_CHECK(std::fabs(innovation.normalised()) > 0.9);

  // A deviation of zero is a filter claiming perfect knowledge of two different
  // numbers. Dividing by it would give an infinity that spreads.
  const Innovation impossible = innovation_of(Estimate{5.0, 0.0}, Estimate{5.1, 0.0});
  RC_CHECK_EQ(impossible.normalised(), 0.0);
}

RC_TEST("a monitor accumulates what it is given, and nothing before that") {
  ConsistencyMonitor monitor;
  RC_CHECK_EQ(monitor.count(), 0);
  RC_CHECK_EQ(monitor.mean(), 0.0);
  RC_CHECK_EQ(monitor.rms(), 0.0);

  monitor.add(Innovation{2.0, 1.0});
  monitor.add(Innovation{-2.0, 1.0});
  RC_CHECK_EQ(monitor.count(), 2);
  RC_CHECK_NEAR(monitor.mean(), 0.0, 1e-12);   // they cancel
  RC_CHECK_NEAR(monitor.rms(), 2.0, 1e-12);    // and they do not

  // Which is the whole reason to keep both. A filter that alternates between
  // two large errors has a mean that looks perfect.
}

RC_TEST("fusing a late fix as though it were current") {
  std::cout << "\n    odometry at 500 Hz, a 10 cm fix at 10 Hz, 1 m/s, 40 s\n\n";
  std::cout << "    " << std::right << std::setw(10) << "fix age" << std::setw(14)
            << "as current" << std::setw(14) << "projected" << std::setw(14)
            << "rewound" << std::setw(16) << "filter claims" << "\n";

  const Outcome dead = run(Method::none, 0, Motion::steady);
  std::vector<Outcome> naive, projected, rewound;

  for (const int late : {0, 40, 80, 150, 300, 600}) {
    const Outcome a = run(Method::naive, late, Motion::steady);
    const Outcome b = run(Method::projected, late, Motion::steady);
    const Outcome c = run(Method::rewound, late, Motion::steady);
    naive.push_back(a);
    projected.push_back(b);
    rewound.push_back(c);
    std::cout << "    " << std::right << std::fixed << std::setprecision(3)
              << std::setw(9) << late / 1000.0 << "s" << std::setprecision(4)
              << std::setw(14) << a.rms << std::setw(14) << b.rms << std::setw(14)
              << c.rms << std::setw(16) << a.claimed << "\n";
  }

  std::cout << "\n    dead reckoning alone over the same forty seconds: "
            << std::setprecision(4) << dead.rms << "\n";

  // Any fix at all beats dead reckoning, until it is late enough.
  RC_CHECK(naive.front().rms < dead.rms * 0.2);

  // Fused as though current, the estimate is worse the later the fix is, and
  // the damage is about the distance travelled during the delay.
  RC_CHECK(naive.back().rms > naive.front().rms * 10.0);
  RC_CHECK(naive.back().rms > dead.rms);

  // Projected forward, it barely moves.
  RC_CHECK(projected.back().rms < projected.front().rms * 2.0);
  RC_CHECK(projected.back().rms < naive.back().rms * 0.15);

  // Rewinding the filter to the measurement's moment is the textbook answer and
  // this is the textbook answer done the way it is usually first written: it
  // replays the motion since and not the corrections since. Where fixes arrive
  // faster than the latency, that discards every correction in the interval.
  RC_CHECK(rewound.back().rms > projected.back().rms);

  // And this is the part worth stopping over. The filter's own claim about its
  // accuracy is the same in every row: it is a statement about the model it was
  // given, not about the answer it produced.
  for (const Outcome& outcome : naive)
    RC_CHECK_NEAR(outcome.claimed, naive.front().claimed, 0.001);
  RC_CHECK(naive.back().rms > naive.back().claimed * 10.0);
}

RC_TEST("what the consistency monitor can and cannot see") {
  std::cout << "\n    normalised innovation, rms over the run\n\n";
  std::cout << "    " << std::right << std::setw(10) << "fix age" << std::setw(16)
            << "steady 1 m/s" << std::setw(18) << "stop and go" << "\n";

  std::vector<double> steady, varying;
  for (const int late : {0, 80, 300, 600}) {
    const double a = run(Method::naive, late, Motion::steady).monitor_rms;
    const double b = run(Method::naive, late, Motion::stop_and_go).monitor_rms;
    steady.push_back(a);
    varying.push_back(b);
    std::cout << "    " << std::right << std::fixed << std::setprecision(3)
              << std::setw(9) << late / 1000.0 << "s" << std::setw(16) << a
              << std::setw(18) << b << "\n";
  }

  std::cout << "\n    at a constant speed the estimate is dragged into the\n";
  std::cout << "    measurement's own past, where the two agree with each\n";
  std::cout << "    other perfectly and with the world not at all\n";

  // At a steady speed the monitor sees nothing at any latency: the filter and
  // the measurement have both settled into the same wrong moment.
  for (const double value : steady) RC_CHECK_NEAR(value, steady.front(), 0.05);

  // A change of speed changes the lag, and a changing error is the only kind a
  // consistency check can see.
  RC_CHECK(varying.back() > varying.front() * 4.0);
  RC_CHECK(varying[1] > 1.0);
}
