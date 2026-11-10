id: E-TEST-0028
title: Normalising so much that a real change is hidden
match: the time and an address in a log line are replaced and nothing else is
match: expected normalize
platforms: linux, windows
teaches: 05-11-the-expected-output-that-changed
---

## Symptom

A log comparison that was once flaky is made reliable by stripping every number from the lines. It never fails again, and a change that doubles the speed goes through the suite for a month.

## Cause

Stripping all the digits makes the comparison independent of the volatile parts and also of the things that matter. Only the parts that change from run to run for no reason should go.

The suite's rules are narrow: `t=` at the start of a word followed by a number, and `0x` followed by hexadecimal digits. `speed 0.5 at limit 1.0` stays as it is, `target=3 t= none` is untouched since `t=` has no number after it, `0x is not an address` stays, and `soft=2.5` stays because the `t=` is inside a word.

## Fix

Name each volatile thing and replace only that, and let a test list the lines that must not change.
