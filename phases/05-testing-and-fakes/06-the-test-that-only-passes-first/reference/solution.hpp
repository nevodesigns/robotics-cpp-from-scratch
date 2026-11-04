#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <vector>

// A test passes on its own and fails in the suite. Or passes in the suite and
// fails when somebody adds a test above it. Or passes on your machine and
// fails in continuous integration, where the order happened to differ.
//
// Lesson 05-05 was about tests that fail sometimes because the machine is
// unpredictable. This one is about tests that fail predictably, in an order
// nobody chose and nobody wrote down.
//
// The cause is always the same: something outside the test survived it.

// Puts a value back however the scope is left.
//
// Note that this type declares a destructor, which lesson 03-09 said to avoid
// because it suppresses the implicit move. Here that is exactly right: a
// scope guard that could be moved could restore twice or not at all, so the
// copy and move operations are deleted on purpose. The rule of zero is a
// default, and this is one of the cases it does not fit.
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

// A test, reduced to the only thing this lesson needs from one: it runs, and
// it either held or it did not.
using Probe = bool (*)();

// Puts the shared world back to its starting state.
//
// The detector below cannot do this itself. It does not know what the probes
// share, and anything generic enough to guess would be a worse version of the
// Restore above. So the caller says.
using Reset = void (*)();

// How a set of probes behaved in each direction.
struct OrderReport {
  int forwards_passed = 0;
  int backwards_passed = 0;
  std::size_t count = 0;

  // The whole point. A suite whose result depends on the order it ran in has
  // state leaking between its tests, and which test is at fault is a separate
  // question from whether any of them is.
  bool order_dependent() const { return forwards_passed != backwards_passed; }

  bool all_passed() const {
    return forwards_passed == static_cast<int>(count) &&
           backwards_passed == static_cast<int>(count);
  }
};

// Runs them front to back, then back to front, and counts.
//
// The cheapest order dependence detector there is, and it finds the common
// case because leaked state usually flows one way: a test that dirties
// something breaks whatever runs after it, so reversing moves the victim in
// front of the culprit.
//
// It is not a proof. Only every permutation would be that, and there are too
// many. It is one extra run that catches most of them.
//
// The reset is not optional and it is the part that was learned the hard way.
// Written without one, this function runs its backwards pass on whatever the
// forwards pass left behind, so a leaky suite scored 1 forwards and 0
// backwards: the second pass failed both probes, including the one that was
// fine, because it inherited the mess from the first. The numbers still
// differed, so the detection happened to work, and they were not the numbers
// they claimed to be.
//
// A tool for finding shared state cannot be free of shared state by accident.
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

#endif  // LESSON_SOLUTION_HPP
