# The Test That Only Passes First: Shared State Between Tests

> The first version of this lesson's order dependence detector had order
> dependence of its own, and reported numbers that were not true while passing
> the test that asserted them.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 02-03, 03-07, 05-01

## The Problem

A test fails. You run it on its own and it passes. You run the suite and it
fails again, in the same place, every time.

You read the test line by line. Every line is correct.

The same defect wears other clothes. A test that passed for a year starts
failing the day somebody adds an unrelated test above it. A suite that is green
on your laptop is red in continuous integration, where the objects were linked
in a different order. A failure that moves when you comment out a different
test.

Lesson 05-05 was about tests that fail **sometimes**, because the machine is
unpredictable and the test asked it a question about time. These fail
**predictably**, in an order nobody chose and nobody wrote down.

The cause is always the same shape: something outside the test survived the
test before it.

## The Concept

### The one that reports is not the one at fault

Here are two functions. Both are correct in isolation. Together they are a bug.

```cpp
static double calibration_offset = 0.0;

bool dirties_the_offset() {
  calibration_offset = 0.25;       // this test needs a different offset
  return calibration_offset > 0.0; // and it holds
}

bool expects_the_default() {
  return calibration_offset == 0.0; // and so does this one
}
```

Run them front to back and the suite's own output is this:

```
  order           dirties the offset    expects the default
  forwards        passed                FAILED
  backwards       passed                passed
```

`expects_the_default` is the test that goes red, and there is nothing wrong
with it. It is the **victim**. `dirties_the_offset` is the culprit and it
passes, every time, in both directions, which is why nobody suspects it.

This is why the symptom depends on order. A suite is a sequence, the leak flows
forward, and whoever is downstream pays.

### What counts as shared

More things than you would guess, in rough order of how often they are the
answer.

A variable at file or namespace scope, like the one above. A `static` local
inside a function, which is the same thing wearing a smaller scope. A
singleton, including the convenient ones such as a logger or a configuration
object. An environment variable. A file on disk the previous test wrote. A
signal handler installed in lesson 04-06's shape and never taken back out. A
cached value computed once on first use.

The last one is worth dwelling on because it does not look like state. A
function that computes something expensive the first time and remembers it has
a hidden input: whether it has been called before. That is shared between every
test in the process.

### Put it back in a destructor, not at the end

The fix is to restore what you changed. The part that matters is **where** the
restoring happens.

```cpp
bool needs_a_different_offset() {
  calibration_offset = 0.25;
  const bool ok = measure() > 0.0;
  if (!ok) return false;        // and the offset stays 0.25 forever
  calibration_offset = 0.0;
  return true;
}
```

That is lesson 02-03's lesson in a new costume. The early return skips the
cleanup, and the paths that most need the cleanup are exactly the paths that do
not reach the bottom of the function: a failed check, a `break`, a throw.

So the restoring goes in a destructor, and the guard is the whole artifact:

```cpp
template <class T>
class Restore {
 public:
  explicit Restore(T& location) : location_(&location), saved_(location) {}

  Restore(T& location, const T& temporary) : location_(&location), saved_(location) {
    location = temporary;
  }

  ~Restore() { *location_ = saved_; }

  Restore(const Restore&) = delete;
  Restore& operator=(const Restore&) = delete;

  const T& saved() const { return saved_; }

 private:
  T* location_;
  T saved_;
};
```

The two-argument constructor is the common case: a test that needs a particular
configuration and must not impose it on the rest of the suite.

### The rule you learned in 03-09 does not apply here

Lesson 03-09 measured what declaring a destructor costs: it suppresses the
implicit move constructor, so a type that would have been moved gets copied
instead, silently, and that is the reason for the rule of zero.

This type declares a destructor **and** deletes its copy and move operations,
which is the opposite advice, and it is right.

A scope guard restores exactly once. A guard that could be moved could restore
twice, with the second restore writing a stale value over a newer one, or not
at all, if the moved-from object was the one whose destructor ran. Deleting the
operations means any code that tries gets a compiler error instead of a
mystery.

The rule of zero is a default, not a law. The reason to know a rule is to know
where it stops.

### Proving there is nothing left

Restoring is the fix. The second half of the lesson is evidence, because a
green suite proves only that the tests pass in the one order they were run in.

```cpp
using Probe = bool (*)();
using Reset = void (*)();

struct OrderReport {
  int forwards_passed = 0;
  int backwards_passed = 0;
  std::size_t count = 0;

  bool order_dependent() const { return forwards_passed != backwards_passed; }
  bool all_passed() const { ... }
};
```

`Probe` is a test reduced to the only thing this needs from one: it runs, and
it either held or it did not. A function pointer rather than a template
parameter, because the point is to put a list of them in a `std::vector` and
run that list twice.

Run the list forwards, reset, run it backwards, compare. The suite's measured
output:

```
  suite                     forwards   backwards   order dependent
  sharing a value                  1           2               YES
  restoring it                     2           2                no
```

Two probes, same code, two different answers depending on direction. That gap
is the detection. Guard the dirtying probe with a `Restore` and the gap closes.

What reversing buys you, and what it does not.

It **catches the common case**, because leaked state flows forward. A test that
dirties something breaks whatever runs after it, so reversing moves the victim
in front of the culprit, where it passes.

It is **not a proof**. Only every permutation would be, and there are `n!` of
them: two orders out of twenty-four for four tests is not coverage. It is one
extra run that finds most real leaks for the price of running the suite twice,
which is why it belongs in continuous integration rather than in an argument
about completeness.

Either answer is informative. Different counts mean a leak exists. Equal counts
mean these two orders agree, which is weaker than no leak and a great deal
stronger than one order.

### The detector leaked

Written the obvious way, `run_in_both_orders` takes a list of probes and runs
it twice. That version is in the lesson too, and here is what it reported for
the same leaky pair that scores 1 and 2:

```
  the same leaky pair with no reset between the passes: 1 forwards, 0 backwards
```

One and zero. The true answer is one and two.

The backwards pass began on whatever the forwards pass left behind. The
forwards pass ran `dirties_the_offset` and left the offset at 0.25, so the
backwards pass ran both probes against a dirty value and both failed, including
the one that would have passed on its own.

The detection still worked. `order_dependent()` compares two numbers for
inequality, and 1 and 0 are unequal just as 1 and 2 are, so the test asserting
order dependence passed. It passed for the wrong reason, reporting figures that
were not true, and the only reason it was caught is that this curriculum prints
what it measures and reads it.

A tool for finding shared state had shared state between its own two passes, in
exactly the shape of the defect it exists to find.

Hence the `Reset` parameter, called before **each** pass. Before the first one
too: without that, the function's answer depends on what ran before the
function, which is the same bug one level out.

The reset is a parameter rather than something the detector does itself because
the detector cannot know what the probes share. Anything generic enough to
guess would be a worse `Restore`, so the caller, who knows, says.

## Build It

Three pieces, in this order.

Write `Restore` with its two constructors and the destructor, and delete copy
and move. Check the obvious case: a value changed inside a scope and back
outside it.

Check nesting next. Three guards on the same variable at 1, 2 and 3 should
unwind to 2, then 1, then the original, each one putting back what it
personally found rather than what the outermost one found. The suite proves
this, and it falls out of destructors running in reverse order of construction.

Then `run_in_both_orders`. Reset, count forwards, reset, count backwards.
Counting down from `size()` to 1 and indexing at `i - 1` keeps an unsigned loop
counter from passing below zero, which is lesson 01-07.

## Use It

```cpp
#include <rc/test/restore.hpp>

static double calibration_offset = 0.0;
static void reset_the_offset() { calibration_offset = 0.0; }

bool guarded() {
  const rc::test::Restore<double> keep(calibration_offset, 0.25);
  return calibration_offset > 0.0;
}

// ... in the suite:
const std::vector<rc::test::Probe> probes = {guarded, expects_the_default};
const rc::test::OrderReport report = rc::test::run_in_both_orders(probes, reset_the_offset);
RC_CHECK(!report.order_dependent());
RC_CHECK(report.all_passed());
```

`Restore` is a template and the suite exercises it on a `double`, a
`std::string`, a `std::vector<int>` and an `int`, because a guard that only
works for arithmetic types is a guard you will hand-write again next week.

## What Breaks First

`E-TEST-0011` is the symptom: a test that passes alone and fails in the suite,
with the full list of what counts as shared and why renaming tests until the
order comes out right is the fix that is not one.

`E-TEST-0012` is the detector, with the measured table and the honest account
of what two orders out of `n!` does and does not establish.

`E-TEST-0013` is the detector's own leak, kept because the defect was invisible
from the outside and the lesson is about exactly that.

## Ship It

`rc/test/restore.hpp`, in `namespace rc::test`.

From here on, any test in this curriculum that touches shared state guards it.
Lesson 04-06's signal handler test hand-rolled this exact type under the name
`HandlerFor` before it existed; that is the signal that it belonged in the
library.
