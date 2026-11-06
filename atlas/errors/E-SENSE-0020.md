id: E-SENSE-0020
title: A speed made from one difference of an encoder, jumping between zero and a thousand
match: no single way of getting speed from counts is good at every speed
match: expected this to be true: e.0..diff1
platforms: linux, windows
teaches: 15-09-speed-from-counts
---

## Symptom

The speed read from an encoder jumps between zero and a big number when the joint is turning slowly, and the loop that uses
it shakes, though the joint is moving smoothly. At a higher speed the shake is gone. A filter on the speed seems to help and
adds a delay that puts the loop out.

## Cause

The speed was the difference of two successive counts over the sample time. A sample holds a whole number of counts. At half
a count per sample it holds none or one, so the speed it reports is zero or one count per sample: a hundred percent out
however smooth the motion is.

Measured, an encoder of 1000 counts a revolution read at 1 kHz, the rms error in the speed as a percent of the speed:

| speed | counts a second | one sample | window of 50 | edge timing | the two combined |
|---|---|---|---|---|---|
| 0.05 rev/s | 50 | 435.9 % | 20.0 % | 1.6 % | 1.6 % |
| 0.50 rev/s | 500 | 100.0 % | 2.0 % | 1.6 % | 1.6 % |
| 5.00 rev/s | 5000 | 14.1 % | 0.3 % | 3.9 % | 0.3 % |
| 50.00 rev/s | 50000 | 1.4 % | 0.0 % | 54.1 % | 0.0 % |

At 0.05 revolutions a second one count arrives every 20 samples, so the single difference reads 1000 counts a second for
one sample and zero for the other nineteen, and its rms error is more than four times the speed.

## Fix

A longer window, or the time between counts, depending on the speed. See `E-SENSE-0021` and `E-SENSE-0022`: no single method
is good at every speed, and the fix is to use each where it is. A velocity loop that wants a good speed at a crawl, such as the
inner loop of lesson 14-09, needs the edge timing, and one that runs at speed needs the window.
