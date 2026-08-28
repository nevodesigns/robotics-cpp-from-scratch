id: E-CTRL-0004
title: A timeout comparison with the wrong sign
match: expected .*(timeout|expired)
platforms: linux, windows
teaches: 14-02-limits-and-watchdog
---

## Symptom

A watchdog either never expires, or expires immediately and permanently.

## Cause

The comparison is inverted or the operands are reversed. The elapsed time is
now minus the last feed, and the watchdog has expired when that elapsed time is
greater than the timeout. Writing last_fed minus now, or using less than, gives
one of the two broken behaviours.

## Fix

Write the elapsed time explicitly as a named variable, then compare it against
the timeout. Naming the intermediate value makes the sign obvious to the reader
and to you. Test both sides of the boundary, which the lesson tests do.
