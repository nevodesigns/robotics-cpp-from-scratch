// rc/test/restore.hpp
//
// A scope guard that puts a shared value back, and a detector for the suites
// that need one, from lesson 05-06, graduated.
//
// A test passes on its own and fails in the suite. Or passes in the suite and
// fails when somebody adds a test above it. Or passes on a laptop and fails in
// continuous integration, where the order happened to differ. The cause is
// always the same shape: something outside the test survived it.
//
// Lesson 05-05 covered tests that fail sometimes because the machine is
// unpredictable. These fail predictably, in an order nobody chose and nobody
// wrote down.
//
// Restore is the fix: it remembers a value on the way in and puts it back on
// the way out, including on an early return, a break, or a throw. It declares
// a destructor, which lesson 03-09 said to avoid because that suppresses the
// implicit move. Here that is the point. A guard that could be moved could
// restore twice or not at all, so copy and move are deleted deliberately. The
// rule of zero is a default, not a law.
//
// run_in_both_orders is the detector: run the probes front to back, reset,
// run them back to front, and compare the counts. Leaked state usually flows
// one way, because a test that dirties something breaks whatever runs after
// it, so reversing the order moves the victim in front of the culprit. It is
// not a proof, since only every permutation would be that, and there are too
// many. It is one extra run that catches the common case.
//
// Measured by the lesson's own suite: two probes sharing a value scored 1
// forwards and 2 backwards, flagged order dependent. The same pair with the
// dirtying probe guarded by a Restore scored 2 and 2, not order dependent.
//
// The reset parameter is not decoration and it was learned the hard way.
// Written without one, the first version of this function ran its backwards
// pass over whatever the forwards pass left behind. That same leaky pair then
// scored 1 forwards and 0 backwards: the second pass failed both probes,
// including the one that was fine, because it inherited the mess from the
// first. The numbers still differed, so the detection still happened to work,
// and they were not the numbers they claimed to be.
//
// A tool for finding shared state cannot be free of shared state by accident.
//
// The caller supplies the reset because this header cannot know what the
// probes share, and anything generic enough to guess would be a worse Restore.

#ifndef RC_TEST_RESTORE_HPP
#define RC_TEST_RESTORE_HPP

#include <cstddef>
#include <vector>

namespace rc {
namespace test {

// Puts a value back however the scope is left.
template <class T>
class Restore {
 public:
  // Remembers the value, and leaves it alone.
  explicit Restore(T& location) : location_(&location), saved_(location) {}

  // Remembers the value, and installs another for the duration of the scope.
  Restore(T& location, const T& temporary) : location_(&location), saved_(location) {
    location = temporary;
  }

  ~Restore() { *location_ = saved_; }

  Restore(const Restore&) = delete;
  Restore& operator=(const Restore&) = delete;

  // What it will put back, which a test can assert on.
  const T& saved() const { return saved_; }

 private:
  T* location_;
  T saved_;
};

// A test, reduced to the only thing the detector needs from one: it runs, and
// it either held or it did not.
using Probe = bool (*)();

// Puts the shared world back to its starting state.
using Reset = void (*)();

// How a set of probes behaved in each direction.
struct OrderReport {
  int forwards_passed = 0;
  int backwards_passed = 0;
  std::size_t count = 0;

  // A suite whose result depends on the order it ran in has state leaking
  // between its tests. Which test is at fault is a separate question from
  // whether any of them is.
  bool order_dependent() const { return forwards_passed != backwards_passed; }

  bool all_passed() const {
    return forwards_passed == static_cast<int>(count) &&
           backwards_passed == static_cast<int>(count);
  }
};

// Runs them front to back, then back to front, and counts. Resets before each
// pass, so the second pass starts where the first one did.
inline OrderReport run_in_both_orders(const std::vector<Probe>& probes, Reset reset) {
  OrderReport report;
  report.count = probes.size();

  reset();
  for (std::size_t i = 0; i < probes.size(); ++i)
    if (probes[i]()) ++report.forwards_passed;

  reset();
  for (std::size_t i = probes.size(); i > 0; --i)
    if (probes[i - 1]()) ++report.backwards_passed;

  return report;
}

}  // namespace test
}  // namespace rc

#endif  // RC_TEST_RESTORE_HPP
