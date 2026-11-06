id: E-CTRL-0017
title: A delay of the wrong length, and an angle read as a quantity
match: a delay line returns what went in some samples ago, and is a wire for none
match: the phase margin is what is left of 180 degrees, and agrees with the complex number
platforms: linux, windows
teaches: 14-07-the-delay-in-the-loop
---

## Symptom

Two quiet ones. A delay of three samples behaves as two or four, so every prediction made
with it is a little off and the loop is stable where the margin says it is not. And a
phase margin of minus 293 degrees is compared with one computed another way and reported
as a mismatch of 360, when both are the same direction.

## Cause

**A delay line is off by one.** A ring buffer that writes before it reads returns the value
it has just stored, which is no delay at all. One that reads and then writes returns the
value from `n` pushes ago, which is right. The error shows only at the edges: the first
value out, a delay of one, and a delay of zero, where the buffer is empty and there is
nothing to index. The test pushes a counting sequence and checks the whole of the first
seven outputs, a fill value before the line has filled, a single sample go round a thousand
times, and the wire.

**An angle compared as a number.** A phase is a direction. Minus 293 degrees and plus 67
point the same way, and the complex number's argument is always reduced to one turn. A
check that compares them directly fails on a correct answer, and one written to pass by
loosening the tolerance would pass a wrong one too.

## Fix

Read before writing in the ring, and count the samples a value spends in the line:

```cpp
const double out = buffer_[head_];
buffer_[head_] = x;
head_ = (head_ + 1) % buffer_.size();
return out;
```

with the empty buffer answered before any of it. And compare angles round the circle:

```cpp
double difference = a - b;
while (difference > 180.0) difference -= 360.0;
while (difference < -180.0) difference += 360.0;
```
