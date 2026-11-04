id: E-TEST-0012
title: A suite that only passes in one order
match: run the suite backwards and compare
platforms: linux, windows
teaches: 05-06-the-test-that-only-passes-first
---

## Symptom

The suite is green. You have no evidence that it would still be green if the
tests ran in a different order, and nothing in it is checking.

The failure arrives later, from somewhere else: a new test inserted in the
middle, a build system that links objects in a different order, a parallel
runner that interleaves, or a colleague who sorts the list alphabetically.

## Cause

A green suite proves that the tests pass in the one order they were run in. If
state leaks between them, that order is part of the answer, and nothing in the
suite says so.

The leak and the order that hides it arrive together, because the test was
written and debugged in the order the file happens to list. Order dependence is
invisible from inside a single order.

## Fix

Run them both ways and compare the counts.

```cpp
#include <rc/test/restore.hpp>

const std::vector<rc::test::Probe> probes = {dirties_the_offset, expects_the_default};
const rc::test::OrderReport report = rc::test::run_in_both_orders(probes, reset_the_offset);
RC_CHECK(!report.order_dependent());
```

Measured in lesson 05-06, with two probes sharing one `double`:

| suite | forwards | backwards | order dependent |
| --- | --- | --- | --- |
| sharing a value | 1 | 2 | YES |
| the dirtying probe guarded by `Restore` | 2 | 2 | no |

Forwards, the probe that dirties the offset runs first and the second one finds
the mess, so one of two passes. Backwards, the one expecting a clean offset runs
before anything has dirtied it, and both pass. Same two probes, same code, two
different answers, which is the detection.

Why reversing is enough to be worth doing, and not enough to be a proof.

**It catches the common case** because leaked state flows forward. A test that
dirties something breaks whatever runs after it, and reversing the order moves
the victim in front of the culprit.

**It is not a proof.** Only running every permutation would be, and there are
`n!` of them. Two orders out of twenty-four for four tests is not coverage. It
is one extra run that finds most real leaks for the cost of running the suite
twice, which is why it belongs in continuous integration rather than in an
argument about completeness.

**Pass or fail, it tells you something.** Different counts mean a leak exists.
Equal counts mean these two orders agree, which is weaker than no leak and much
stronger than one order.
