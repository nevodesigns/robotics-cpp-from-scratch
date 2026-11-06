id: E-SENSE-0022
title: Edge timing that holds its last speed, flips on a reversal, or fails when fast
match: a stopped encoder reads slower and slower, and not its last speed for ever
match: the time between two counts is the speed, with a sign, and a reversal is not a speed
platforms: linux, windows
teaches: 15-09-speed-from-counts
---

## Symptom

A joint stops and the speed estimate stays at the speed it had, for as long as nothing moves. Or a joint reverses and the
estimate shows a huge speed in the wrong direction for a moment. Or, at high speed, an estimate that was accurate when slow is
half wrong. Each is the same method failing in a different regime.

## Cause

Measuring the time between two counts is the best way to read a slow speed: a clock tick of 10 microseconds against a gap of
20 milliseconds is a twentieth of a percent. Three cases break it.

**A stopped encoder produces no edges.** A speed computed from the last two edges is a fact about the past, and holds for ever.
If more than a gap has passed since the last edge, the speed can only be lower, at most one count over the time since the last
one, and that is the estimate to use: 500 counts a second with a gap of 2 ms reads 10 a tenth of a second after the last
edge and 1 after a second.

**A reversal is not a speed.** The first edge after turning round is the same position crossed the other way, and the time
since the previous edge says nothing about how fast it is going. The first edge in a new direction has nothing to pair with, so
the estimate is zero until the second, and then it is signed the other way: -250 counts a second after a gap of 4 ms in the new
direction.

**A fast encoder has gaps of a few timer ticks.** The same clock tick is now a large part of the gap. Measured at 50 revolutions
a second, a gap of 20 microseconds against a tick of 10, the edge timing was **54.1 percent** out, where at 0.5 revolutions a
second it was 1.6.

## Fix

Bound the estimate by the time since the last edge, forget the previous edge on a reversal, and do not use edge timing above a
threshold. `pick_velocity` takes the edge timing when it exists and is below 1500 counts a second, and the window otherwise,
and the result is within 2 percent at every speed in the table.
