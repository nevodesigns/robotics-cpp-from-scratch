id: E-MATH-0006
title: A velocity differenced from a noisy position
match: a millimetre of position is most of a metre per second
match: widening the interval beats filtering what came out of it
platforms: linux, windows
teaches: 06-05-the-derivative-you-compute
---

## Symptom

A velocity computed from position samples is unusable: it jumps by metres per
second while the robot is crawling, and a controller acting on it lurches.

The position measurements look fine. They are accurate to a millimetre.

## Cause

Differencing amplifies noise by the reciprocal of the interval, and a control
loop's interval is small.

Two samples are subtracted, so their noise adds in quadrature to `sigma * sqrt(2)`,
and the whole thing is divided by the interval. At 500 Hz:

| position noise | velocity rms measured | sigma*sqrt(2)/dt |
|---|---|---|
| 0.1 mm | 0.0705 m/s | 0.0707 |
| **1 mm** | **0.7049 m/s** | 0.7071 |
| 10 mm | 7.0487 m/s | 7.0711 |

One millimetre of position noise, differenced over two milliseconds, is seven
tenths of a metre per second. That is not a defect in the sensor and not a bug
in the arithmetic; it is what differencing does.

Note which way the step size goes here. For a function you can call, a smaller
step is better until floating point stops you (`E-NUM-0008`). For measurements,
a smaller interval is **always worse**, and the two problems are constantly
confused because they use the same three symbols.

## Fix

Widen the interval rather than filtering the result.

```cpp
const double velocity = rc::math::rate_over(older_sample, newest_sample, span * dt);
```

Differencing over N samples divides the noise by **N**, because the numerator's
noise does not grow while the denominator does:

| N | lag | velocity rms | times better |
|---|---|---|---|
| 1 | 0.001 s | 0.7049 | 1.0 |
| 16 | 0.016 s | 0.0441 | 16.0 |
| 256 | 0.256 s | 0.0028 | 254.9 |

Averaging the noisy velocity over the same window would divide it by the square
root of N, which at 256 is sixteen rather than two hundred and fifty six, **for
the same delay**. Both cost `N * dt / 2` of lag, and one of them buys sixteen
times more for it.

So the order of operations is: difference over as wide an interval as the lag
budget allows, and filter afterwards only if that is still not enough. Doing it
the other way round, which is the instinct, wastes most of the budget.

Then predict rather than tune:

```cpp
const double expected = rc::math::rate_noise(sensor_noise, span * dt);
```

Compare that against what the loop can tolerate, and against the lag budget in
lesson 15-03, before writing the loop rather than after it misbehaves.

Where an interval wide enough is not affordable, the answer is not a longer
difference. It is to stop differencing: an estimator that fuses position with a
motion model produces a velocity without amplifying anything, which is what
phase 16 is about.
