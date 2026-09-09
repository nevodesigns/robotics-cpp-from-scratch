#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <chrono>
#include <thread>

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
  // TODO 1: run the check that many times and count.
  //
  // Increment runs every time, and failures when the check returns false.
  //
  // This is the instrument. A flaky test is not a test that is wrong, it is a
  // test whose result is a random variable, and the first thing to do with one
  // of those is measure its distribution rather than sample it once and rerun.
  (void)times;
  (void)check;
  return Trials{};
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
  // TODO 2: the smallest of several measurements.
  //
  // Call measure that many times and keep the smallest. Return -1.0 when asked
  // for none, rather than a number that means nothing.
  //
  // The minimum is the only statistic a microbenchmark on a shared machine can
  // defend: noise can add time and never remove it, so the smallest sample is
  // the closest any run came to measuring the thing itself.
  (void)times;
  (void)measure;
  return -1.0;
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
  // TODO 3: wait for the condition, or give up at the deadline.
  //
  // Work out the deadline once, from steady_clock::now(). Then loop: return
  // true the moment the condition holds, return false once the deadline has
  // passed, and sleep for poll_seconds between looks so the wait does not spin.
  //
  // Check the condition before the deadline, so something already true costs
  // nothing.
  //
  // The alternative is sleeping a guessed length of time, which is wrong in
  // both directions at once: too short and the test fails on a busy machine,
  // too long and every run pays for it. A machine four times slower needs a
  // sleep four times longer and there is no number that is both.
  (void)condition;
  (void)seconds;
  (void)poll_seconds;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
