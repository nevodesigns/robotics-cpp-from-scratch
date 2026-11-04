id: E-TEST-0011
title: The test that only passes first
match: a test that passes alone and fails in the suite
platforms: linux, windows
teaches: 05-06-the-test-that-only-passes-first
---

## Symptom

A test fails. Run it on its own and it passes. Run the whole suite and it fails
again, in the same place, every time.

Variations, all the same defect:

A test that passed for a year starts failing the day somebody adds an unrelated
test above it.

A suite that is green on a laptop and red in continuous integration, where the
files were compiled in a different order.

A failure that moves when you comment out a different test.

Nothing in the failing test is wrong. You can read it line by line and every
line is correct.

## Cause

Something outside the test survived the test that ran before it.

The usual candidates, in rough order of how often they are the answer:

A variable at file or namespace scope. A `static` local inside a function. A
singleton, including the convenient ones such as a logger or a configuration
object. An environment variable. A file on disk the previous test wrote. A
signal handler installed by lesson 04-06's shape and never taken back out. A
cached value computed once.

The failing test is not the culprit. It is the victim, and it is the one that
gets blamed because it is the one that reported. The culprit ran earlier,
changed something, and passed.

This is why the symptom depends on order. A suite is a sequence, the leak flows
forward, and whoever is downstream of it pays.

## Fix

Two moves, and the first is the one that matters.

**Put it back.** Any test that changes shared state restores it, and the
restoring happens in a destructor rather than at the end of the test body,
because an early return, a `RC_CHECK` that bails, a `break` or a throw all skip
a line at the end and none of them skip a destructor.

```cpp
#include <rc/test/restore.hpp>

static double calibration_offset = 0.0;

bool needs_a_different_offset() {
  const rc::test::Restore<double> keep(calibration_offset, 0.25);
  // calibration_offset is 0.25 for the rest of this scope, and 0.0 after it,
  // whichever way the scope is left.
  return measure() > 0.0;
}
```

`Restore` deletes its copy and move operations deliberately. A guard that could
be moved could restore twice, or not at all. This is the case lesson 03-09's
rule of zero does not fit, and the reason to know a rule is to know when it
stops applying.

**Then prove there is nothing left.** Run the suite in reverse and compare
(`E-TEST-0012`).

The move that does not work is renaming tests so the order comes out right. It
holds until the next person adds one, and it hides the leak instead of removing
it, so the next failure is further from its cause than this one was.
