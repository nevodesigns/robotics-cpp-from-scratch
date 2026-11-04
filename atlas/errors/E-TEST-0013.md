id: E-TEST-0013
title: The order dependence detector that leaked
match: the detector inherited the state it was looking for
platforms: linux, windows
teaches: 05-06-the-test-that-only-passes-first
---

## Symptom

A function runs a list of tests forwards, then backwards, and compares the
counts. It reports order dependence correctly. The counts it prints are wrong.

In lesson 05-06, with a known leaky pair of probes, the first version of
`run_in_both_orders` reported **1 forwards and 0 backwards**. The true answer is
1 and 2.

The bug is invisible from the outside, because 1 and 0 differ just as 1 and 2
do, so `order_dependent()` returned `true` either way and the test asserting it
passed.

## Cause

There was no reset between the two passes, so the backwards pass began on
whatever the forwards pass left behind.

The forwards pass ran the probe that dirties the shared value, leaving it
dirty. The backwards pass then ran both probes against an already dirty value,
and both failed, including the one that would have passed on its own. Hence
zero.

A tool for detecting shared state had shared state of its own, between its own
two passes, and the shape was identical to the defect it exists to find.

This is the general trap with any measurement that runs the thing twice: pass
two does not start where pass one did unless something puts it back. Lesson
04-05's benchmark harness has it (a timing section that does not clear its slots
feeds the previous run's totals to the next one), and so does every
before-and-after comparison.

## Fix

The caller supplies a reset, and the detector calls it before each pass.

```cpp
inline OrderReport run_in_both_orders(const std::vector<Probe>& probes, Reset reset) {
  OrderReport report;
  report.count = probes.size();

  reset();
  for (std::size_t i = 0; i < probes.size(); ++i)
    if (probes[i]()) ++report.forwards_passed;

  reset();                 // without this line the counts are 1 and 0
  for (std::size_t i = probes.size(); i > 0; --i)
    if (probes[i - 1]()) ++report.backwards_passed;

  return report;
}
```

The reset is a parameter rather than something the function does itself because
the function cannot know what the probes share. Anything generic enough to guess
would be a worse `Restore`, so the caller, who knows, says.

Two things worth taking from this.

**The first reset matters too.** Without it the forwards pass inherits whatever
the previous caller left, so the function's answer depends on what ran before
it: the same defect one level out.

**A test that passes is not a test that is right.** The assertion here was
`order_dependent() == true`, and it held for the wrong reason. The numbers were
only caught by printing them and reading them, which is why this curriculum
prints what it measures instead of asserting a boolean and moving on.
