#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

using Clock = std::chrono::steady_clock;

// A piece of work whose time is worth measuring: enough to take a few
// microseconds and nothing that allocates or waits.
double busy_microseconds() {
  const auto start = Clock::now();
  double total = 0.0;
  for (int i = 1; i <= 20000; ++i) total += 1.0 / i;
  if (total == 12345.0) std::cout << "";
  return std::chrono::duration<double, std::micro>(Clock::now() - start).count();
}

}  // namespace

RC_TEST("counting failures instead of rerunning until green") {
  // A check that fails exactly one time in ten, so the instrument can be
  // checked against an answer that is known rather than measured.
  int attempt = 0;
  const Trials one_in_ten = repeat(200, [&attempt] { return (++attempt % 10) != 0; });

  RC_CHECK_EQ(one_in_ten.runs, 200);
  RC_CHECK_EQ(one_in_ten.failures, 20);
  RC_CHECK_NEAR(one_in_ten.failure_rate(), 0.1, 1e-12);

  // Something that always passes, and something that never does.
  const Trials always = repeat(50, [] { return true; });
  RC_CHECK_EQ(always.failures, 0);
  RC_CHECK_EQ(always.failure_rate(), 0.0);

  const Trials never = repeat(50, [] { return false; });
  RC_CHECK_EQ(never.failures, 50);
  RC_CHECK_NEAR(never.failure_rate(), 1.0, 1e-12);

  // No runs is not a division by zero.
  const Trials none = repeat(0, [] { return true; });
  RC_CHECK_EQ(none.runs, 0);
  RC_CHECK_EQ(none.failure_rate(), 0.0);

  std::cout << "\n    a test that fails sometimes is usually rerun until it\n";
  std::cout << "    passes, which throws away the only information anybody has\n";
  std::cout << "    about it. One failure in a thousand and one in three are\n";
  std::cout << "    very different faults and a red run followed by a green one\n";
  std::cout << "    cannot tell them apart\n";
}

RC_TEST("the smallest of several, on numbers that are known") {
  // best_of is tested against a sequence it cannot argue with, because a test
  // of a timing tool must not itself depend on timing.
  double values[] = {9.0, 4.0, 7.0, 4.5, 12.0};
  int index = 0;
  RC_CHECK_NEAR(best_of(5, [&index, &values] { return values[index++]; }), 4.0, 1e-12);

  index = 0;
  RC_CHECK_NEAR(best_of(1, [&index, &values] { return values[index++]; }), 9.0, 1e-12);

  // Fewer samples can never find a smaller minimum, which is the whole property
  // being relied on.
  index = 0;
  const double one = best_of(1, [&index, &values] { return values[index++]; });
  index = 0;
  const double five = best_of(5, [&index, &values] { return values[index++]; });
  RC_CHECK(five <= one);

  // No samples is not a number.
  RC_CHECK_EQ(best_of(0, [] { return 1.0; }), -1.0);
}

RC_TEST("what a single timing measurement is actually measuring") {
  const int samples = 400;
  std::vector<double> times;
  times.reserve(samples);
  for (int i = 0; i < samples; ++i) times.push_back(busy_microseconds());

  double total = 0.0;
  for (const double value : times) total += value;
  const double mean = total / samples;

  std::sort(times.begin(), times.end());
  const double fastest = times.front();
  const double median = times[samples / 2];
  const double ninety_ninth = times[static_cast<std::size_t>(samples * 0.99)];
  const double worst = times.back();

  std::cout << "\n    the same work, timed " << samples << " times\n\n";
  std::cout << "    " << std::left << std::setw(24) << "fastest" << std::right
            << std::fixed << std::setprecision(2) << fastest << " us\n";
  std::cout << "    " << std::left << std::setw(24) << "median" << std::right
            << median << " us\n";
  std::cout << "    " << std::left << std::setw(24) << "mean" << std::right << mean
            << " us\n";
  std::cout << "    " << std::left << std::setw(24) << "99th percentile"
            << std::right << ninety_ninth << " us\n";
  std::cout << "    " << std::left << std::setw(24) << "worst" << std::right << worst
            << " us\n";

  // A distribution with a floor and no ceiling: nothing can make the work
  // faster than it is, and a great many things can make it slower. Those three
  // relations hold on any machine, which is why they are the only ones asserted
  // here.
  RC_CHECK(fastest <= median);
  RC_CHECK(worst >= ninety_ninth);
  RC_CHECK(mean >= fastest);

  std::cout << "\n    so a threshold placed anywhere near the middle is a coin\n";
  std::cout << "    toss, and taking the smallest of several samples moves the\n";
  std::cout << "    measurement toward the floor, where it is repeatable\n";
}

RC_TEST("the machine is not the same machine ten seconds later") {
  std::cout << "\n    the same work, in blocks of 200, through one run\n\n";
  std::cout << "    " << std::right << std::setw(8) << "block" << std::setw(12)
            << "median" << std::setw(12) << "fastest" << "\n";

  std::vector<double> medians;
  for (int block = 0; block < 6; ++block) {
    std::vector<double> times;
    times.reserve(200);
    for (int i = 0; i < 200; ++i) times.push_back(busy_microseconds());
    std::sort(times.begin(), times.end());
    medians.push_back(times[100]);
    std::cout << "    " << std::right << std::setw(8) << block << std::fixed
              << std::setprecision(2) << std::setw(12) << times[100] << std::setw(12)
              << times.front() << "\n";
  }

  std::cout << "\n    the numbers above depend on the machine and on what it\n";
  std::cout << "    was doing. Run alone, this work measured 41.21 microseconds\n";
  std::cout << "    in its first block and 48.79 by its fifth, with the fastest\n";
  std::cout << "    sample rising from 38.38 to 47.47: the machine changing\n";
  std::cout << "    state, not noise. Here the earlier tests have already warmed\n";
  std::cout << "    it, so the rise is smaller, which is the same point made\n";
  std::cout << "    twice\n";
  std::cout << "\n    which is why nothing above is asserted. A threshold\n";
  std::cout << "    calibrated in a warm up measures a machine that no longer\n";
  std::cout << "    exists by the time the test runs, and no amount of taking\n";
  std::cout << "    the smallest of several undoes a systematic shift. The only\n";
  std::cout << "    safe assertion about a benchmark is a comparison measured\n";
  std::cout << "    now, never a time measured then\n";

  // Six blocks were measured and each produced a number. That is all this test
  // claims, on purpose: a test about flaky tests must not be one.
  RC_CHECK_EQ(static_cast<int>(medians.size()), 6);
  for (const double median : medians) RC_CHECK(median > 0.0);
}

RC_TEST("waiting for a thing, not for a length of time") {
  std::atomic<bool> finished{false};
  std::atomic<int> produced{0};

  std::thread worker([&finished, &produced] {
    double total = 0.0;
    for (int i = 1; i <= 200000; ++i) total += 1.0 / i;
    produced.store(static_cast<int>(total));
    finished.store(true);
  });

  const auto start = Clock::now();
  const bool arrived = wait_until([&finished] { return finished.load(); }, 5.0);
  const double waited =
      std::chrono::duration<double, std::milli>(Clock::now() - start).count();
  worker.join();

  std::cout << "\n    " << std::left << std::setw(34) << "the work finished"
            << std::right << (arrived ? "yes" : "no") << "\n";
  std::cout << "    " << std::left << std::setw(34) << "waited" << std::right
            << std::fixed << std::setprecision(2) << waited << " ms\n";
  std::cout << "    " << std::left << std::setw(34) << "deadline" << std::right
            << "5000.00 ms\n";

  RC_CHECK(arrived);
  RC_CHECK(produced.load() > 0);

  // It returned when the work was done rather than when a guessed interval
  // elapsed, so the common case costs what the work cost and the deadline is
  // there only to fail rather than hang.
  RC_CHECK(waited < 5000.0);

  // A condition that never becomes true gives up, and says so, instead of
  // hanging the suite. That is the other half of why a deadline exists.
  const auto gave_up_at = Clock::now();
  const bool never = wait_until([] { return false; }, 0.05);
  const double spent =
      std::chrono::duration<double, std::milli>(Clock::now() - gave_up_at).count();
  RC_CHECK(!never);
  RC_CHECK(spent >= 45.0);

  // Something already true costs nothing at all.
  RC_CHECK(wait_until([] { return true; }, 5.0));

  std::cout << "\n    a fixed sleep is wrong in both directions at once: too\n";
  std::cout << "    short and it fails on a busy machine, too long and every\n";
  std::cout << "    run pays for it. A machine four times slower needs a sleep\n";
  std::cout << "    four times longer, and there is no number that is both\n";
}

RC_TEST("the same seed, the same answer") {
  // The other source of a result that changes between runs, from lesson 05-04:
  // a generator nobody seeded. Here for completeness, because a flaky test is
  // either time or randomness and it is worth knowing which before looking.
  const auto sequence = [](std::uint64_t seed) {
    std::vector<int> values;
    std::uint64_t state = seed * 6364136223846793005ULL + 1ULL;
    for (int i = 0; i < 20; ++i) {
      state = state * 6364136223846793005ULL + 1442695040888963407ULL;
      values.push_back(static_cast<int>((state >> 33) % 1000));
    }
    return values;
  };

  RC_CHECK(sequence(7) == sequence(7));
  RC_CHECK(sequence(7) != sequence(8));

  std::cout << "\n    a test whose result changes between runs is either time\n";
  std::cout << "    or randomness. Randomness is fixed by a seed that is\n";
  std::cout << "    printed, and time is fixed by measuring rather than\n";
  std::cout << "    guessing\n";
}
