// rc/test/repeat.hpp
//
// Measuring a test that fails sometimes, instead of rerunning it, from lesson
// 05-05.
//
// A test that fails sometimes is usually rerun until it passes, which discards
// the only information anybody has about it. One failure in a thousand and one
// in three are very different faults and a red run followed by a green one
// cannot tell them apart.
//
// The three tools here are the three answers to the three causes.
//
// repeat, because a flaky result is a random variable and the first thing to do
// with one is measure its distribution.
//
// best_of, because a timing distribution has a floor and no ceiling: noise can
// add time and never remove it, so the smallest of several samples is the
// closest any run came to measuring the thing itself.
//
// wait_until, because a fixed sleep is wrong in both directions at once. Too
// short and the test fails on a busy machine, too long and every run pays for
// it, and a machine four times slower needs a sleep four times longer.
//
// What none of them fixes is that the machine changes. Run alone, the same work
// measured 41.21 microseconds in its first block of 200 and 48.79 by its fifth,
// with the fastest sample rising from 38.38 to 47.47. A threshold calibrated in
// a warm up measures a machine that no longer exists, and no amount of taking
// the smallest of several undoes a systematic shift. The only safe assertion
// about a benchmark is a comparison measured now, never a time measured then.

#ifndef RC_TEST_REPEAT
#define RC_TEST_REPEAT

#include <chrono>
#include <thread>

namespace rc {
namespace test {

// How often something failed, out of how many attempts.
//
// A test that fails sometimes is usually rerun until it passes, which discards
// the only information anybody has about it. One failure in a thousand and one
// in three are very different faults, and neither is visible from a single red
// run followed by a green one.
struct Trials {
  int runs = 0;
  int failures = 0;

  double failure_rate() const {
    if (runs == 0) return 0.0;
    return static_cast<double>(failures) / static_cast<double>(runs);
  }
};

// Run something that returns whether it passed, and count.
//
// This is the instrument. A flaky test is not a test that is wrong, it is a
// test whose result is a random variable, and the first thing to do with a
// random variable is measure its distribution rather than sample it once.
template <class Check>
Trials repeat(int times, Check check) {
  Trials trials;
  for (int i = 0; i < times; ++i) {
    ++trials.runs;
    if (!check()) ++trials.failures;
  }
  return trials;
}

// The smallest of several measurements.
//
// The only statistic a microbenchmark on a shared machine can defend. Noise can
// only ever add time, never remove it, so the minimum is the closest any run
// came to measuring the thing itself. A mean measures whatever else the machine
// was doing, and a single sample measures whether it happened to be doing it
// just then.
template <class Measure>
double best_of(int times, Measure measure) {
  double best = -1.0;
  for (int i = 0; i < times; ++i) {
    const double value = measure();
    if (best < 0.0 || value < best) best = value;
  }
  return best;
}

// Wait for something to become true, or give up.
//
// The alternative is sleeping for a guessed length of time, which is wrong in
// both directions at once: too short and the test fails on a busy machine, too
// long and every run pays for it whether or not it needed to. A machine four
// times slower than yours needs a sleep four times longer, and there is no
// number that is both.
//
// This returns as soon as the condition holds, so the common case costs
// whatever the work actually took, and it has a deadline, so a condition that
// never becomes true fails the test rather than hanging the suite.
template <class Condition>
bool wait_until(Condition condition, double seconds, double poll_seconds = 0.0002) {
  const auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::duration<double>(seconds);
  while (true) {
    if (condition()) return true;
    if (std::chrono::steady_clock::now() >= deadline) return false;
    std::this_thread::sleep_for(std::chrono::duration<double>(poll_seconds));
  }
}

}  // namespace test
}  // namespace rc

#endif  // RC_TEST_REPEAT
